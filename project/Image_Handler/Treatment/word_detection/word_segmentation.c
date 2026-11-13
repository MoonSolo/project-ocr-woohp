#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WordExtractionResult extract_individual_words(MagickWand *word_list_region,
                                               const char *debug_folder)
{
    WordExtractionResult result = {false, NULL, 0};
    
    if (!word_list_region)
        return result;
    
    size_t width = MagickGetImageWidth(word_list_region);
    size_t height = MagickGetImageHeight(word_list_region);
    
    MagickWand *processed = CloneMagickWand(word_list_region);
    if (!processed)
        return result;
    
    MagickTransformImageColorspace(processed, GRAYColorspace);
    MagickNormalizeImage(processed);
    MagickThresholdImage(processed, 0.5 * QuantumRange);
    MagickDespeckleImage(processed);
    
    if (debug_folder && debug_folder[0] != '\0')
    {
        char path[2048];
        snprintf(path, sizeof(path),
                 "%s/word_detection/words_preprocessed.png",
                 debug_folder);
        MagickWriteImage(processed, path);
    }
    
    unsigned char *pixels = (unsigned char*)malloc(width * height);
    if (!pixels)
    {
        DestroyMagickWand(processed);
        return result;
    }
    
    MagickExportImagePixels(processed, 0, 0, width, height, "I",
                           CharPixel, pixels);
    DestroyMagickWand(processed);
    
    int *row_sum = (int*)calloc(height, sizeof(int));
    
    for (int y = 0; y < (int)height; y++)
    {
        int sum = 0;
        for (int x = 0; x < (int)width; x++)
        {
            if (pixels[y * width + x] < 128)
                sum++;
        }
        row_sum[y] = sum;
    }
    
    smooth_projection(row_sum, height, 2);
    
    int max_row = 0;
    for (int y = 0; y < (int)height; y++)
    {
        if (row_sum[y] > max_row)
            max_row = row_sum[y];
    }
    
    int row_threshold = max_row * 0.15;
    
    save_projection_visualization(row_sum, height, row_threshold,
                                  debug_folder, "row", 0);
    
    Region text_rows[50];
    int row_count = detect_text_rows(row_sum, height, row_threshold,
                                     text_rows, 50);
    
    row_count = merge_close_rows(text_rows, row_count);
    
    int median_height = calculate_median_height(text_rows, row_count);
    
    WordBounds word_bounds[100];
    int word_count = 0;
    
    for (int i = 0; i < row_count && word_count < 100; i++)
    {
        int h = text_rows[i].end - text_rows[i].start;
        
        if (h < 5)
            continue;
        
        if (h > median_height * 5)
        {
            process_tall_row_by_columns(pixels, width, height,
                                       text_rows[i], median_height,
                                       word_bounds, &word_count, 100);
            continue;
        }
        
        int row_start = text_rows[i].start;
        int row_end = text_rows[i].end;
        int row_height = row_end - row_start;
        
        int *col_sum = (int*)calloc(width, sizeof(int));
        
        for (int x = 0; x < (int)width; x++)
        {
            int sum = 0;
            for (int y = row_start; y < row_end && y < (int)height; y++)
            {
                if (pixels[y * width + x] < 128)
                    sum++;
            }
            col_sum[x] = sum;
        }
        
        smooth_projection(col_sum, width, 1);
        
        int col_threshold = row_height * 0.05;
        
        char proj_name[64];
        snprintf(proj_name, sizeof(proj_name), "col_row_%d", i);
        save_projection_visualization(col_sum, width, col_threshold,
                                      debug_folder, proj_name, 1);
        
        Region words_in_row[50];
        int words_in_row_count = 0;
        
        int in_word = 0;
        for (int x = 0; x < (int)width && words_in_row_count < 50; x++)
        {
            if (col_sum[x] > col_threshold)
            {
                if (!in_word)
                {
                    words_in_row[words_in_row_count].start = x;
                    in_word = 1;
                }
            }
            else if (in_word)
            {
                words_in_row[words_in_row_count].end = x;
                words_in_row_count++;
                in_word = 0;
            }
        }
        
        if (in_word && words_in_row_count < 50)
        {
            words_in_row[words_in_row_count].end = width;
            words_in_row_count++;
        }
        
        words_in_row_count = merge_word_regions(words_in_row,
                                                words_in_row_count);
        
        free(col_sum);
        
        for (int word_idx = 0;
             word_idx < words_in_row_count && word_count < 100;
             word_idx++)
        {
            int word_width = words_in_row[word_idx].end -
                            words_in_row[word_idx].start;
            float aspect_ratio = (float)word_width / (float)row_height;
            
            if (word_width < 15)
                continue;
            
            if (word_width > (int)(width * 0.85f) ||
                aspect_ratio > 20.0f)
                continue;
            
            if (aspect_ratio < 1.5f && word_width > 80)
                continue;
            
            word_bounds[word_count].x = (words_in_row[word_idx].start > 3)?
                                        (words_in_row[word_idx].start - 3) : 0;
            word_bounds[word_count].y = (row_start > 3) ?
                                        (row_start - 3) : 0;
            word_bounds[word_count].width = word_width + 6;
            word_bounds[word_count].height = row_height + 6;
            
            if (word_bounds[word_count].x +
                word_bounds[word_count].width > (int)width)
                word_bounds[word_count].width = width -
                                                word_bounds[word_count].x;
            if (word_bounds[word_count].y +
                word_bounds[word_count].height > (int)height)
                word_bounds[word_count].height = height -
                                                 word_bounds[word_count].y;
            
            word_count++;
        }
    }
    
    free(row_sum);
    free(pixels);
    
    if (word_count == 0)
        return result;
    
    result.words = (MagickWand**)malloc(word_count *
                                        sizeof(MagickWand*));
    if (!result.words)
        return result;
    
    for (int i = 0; i < word_count; i++)
    {
        result.words[i] = CloneMagickWand(word_list_region);
        if (!result.words[i])
        {
            for (int j = 0; j < i; j++)
                DestroyMagickWand(result.words[j]);
            free(result.words);
            result.words = NULL;
            return result;
        }
        
        if (MagickCropImage(result.words[i], 
                           word_bounds[i].width, 
                           word_bounds[i].height,
                           word_bounds[i].x, 
                           word_bounds[i].y) == MagickFalse)
        {
            for (int j = 0; j <= i; j++)
                DestroyMagickWand(result.words[j]);
            free(result.words);
            result.words = NULL;
            return result;
        }
        
        MagickResetImagePage(result.words[i], "");
        
        if (debug_folder && debug_folder[0] != '\0')
        {
            char filename[2048];
            snprintf(filename, sizeof(filename),
                     "%s/word_detection/individual_words/word_%02d.png",
                     debug_folder, i);
            MagickWriteImage(result.words[i], filename);
        }
    }
    
    result.success = true;
    result.word_count = word_count;
    
    return result;
}

void free_word_extraction_result(WordExtractionResult *result)
{
    if (result && result->words)
    {
        for (size_t i = 0; i < result->word_count; i++)
        {
            if (result->words[i])
                DestroyMagickWand(result->words[i]);
        }
        free(result->words);
        result->words = NULL;
    }
}