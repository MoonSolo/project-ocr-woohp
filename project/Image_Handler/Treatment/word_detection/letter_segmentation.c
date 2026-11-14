#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_complete_vertical_gap(unsigned char *pixels, int width,
                                    int height, int x)
{
    for (int y = 0; y < height; y++)
    {
        if (pixels[y * width + x] < 128)
        {
            return 0;
        }
    }
    return 1;
}

static int find_best_split_point(unsigned char *pixels, int width,
                                 int height, int start, int end)
{
    int best_x = -1;
    int max_gap_height = 0;
    
    for (int x = start; x < end; x++)
    {
        int gap_height = 0;
        for (int y = 0; y < height; y++)
        {
            if (pixels[y * width + x] >= 128)
                gap_height++;
        }
        
        if (gap_height > max_gap_height)
        {
            max_gap_height = gap_height;
            best_x = x;
        }
    }
    
    if (max_gap_height >= (int)(height * 0.4))
        return best_x;
    
    return -1;
}

static int detect_letter_boundaries_by_gaps(unsigned char *pixels,
                                            int width, int height,
                                            Region *letter_regions,
                                            int max_letters)
{
    int letter_count = 0;
    int in_letter = 0;
    int letter_start = 0;
    
    for (int x = 0; x < width && letter_count < max_letters; x++)
    {
        int is_gap = is_complete_vertical_gap(pixels, width, height, x);
        
        if (! is_gap)
        {
            if (! in_letter)
            {
                letter_start = x;
                in_letter = 1;
            }
        }
        else
        {
            if (in_letter)
            {
                letter_regions[letter_count].start = letter_start;
                letter_regions[letter_count].end = x;
                letter_count++;
                in_letter = 0;
            }
        }
    }
    
    if (in_letter && letter_count < max_letters)
    {
        letter_regions[letter_count].start = letter_start;
        letter_regions[letter_count].end = width;
        letter_count++;
    }
    
    return letter_count;
}

static int filter_noise_regions(Region *letter_regions, int letter_count,
                                int min_width)
{
    int write_idx = 0;
    
    for (int i = 0; i < letter_count; i++)
    {
        int width = letter_regions[i]. end - letter_regions[i]. start;
        
        if (width >= min_width)
        {
            letter_regions[write_idx] = letter_regions[i];
            write_idx++;
        }
    }
    
    return write_idx;
}

static int split_wide_letters(unsigned char *pixels, int img_width,
                              int img_height, Region *letter_regions,
                              int letter_count, int avg_width)
{
    Region split_regions[100];
    int split_count = 0;
    
    for (int i = 0; i < letter_count && split_count < 100; i++)
    {
        int width = letter_regions[i].end - letter_regions[i].start;
        
        if (width > avg_width * 1.4)
        {
            int split_x = find_best_split_point(pixels, img_width,
                                               img_height,
                                               letter_regions[i].start,
                                               letter_regions[i].end);
            
            if (split_x > 0)
            {
                split_regions[split_count].start = letter_regions[i].start;
                split_regions[split_count].end = split_x;
                split_count++;
                
                if (split_count < 100)
                {
                    split_regions[split_count].start = split_x + 1;
                    split_regions[split_count].end = letter_regions[i].end;
                    split_count++;
                }
            }
            else
            {
                int num_letters = (width + avg_width / 2) / avg_width;
                if (num_letters > 1 && num_letters <= 3)
                {
                    int segment_width = width / num_letters;
                    
                    for (int j = 0; j < num_letters && split_count < 100; j++)
                    {
                        split_regions[split_count].start =
                            letter_regions[i].start + (j * segment_width);
                        split_regions[split_count].end =
                            letter_regions[i].start + ((j + 1) * segment_width);
                        split_count++;
                    }
                }
                else
                {
                    split_regions[split_count++] = letter_regions[i];
                }
            }
        }
        else
        {
            split_regions[split_count++] = letter_regions[i];
        }
    }
    
    if (split_count > 0 && split_count <= 50)
    {
        memcpy(letter_regions, split_regions,
               split_count * sizeof(Region));
        return split_count;
    }
    
    return letter_count;
}

static MagickBooleanType resize_letter_to_64x64(MagickWand *letter)
{
    if (!letter) return MagickFalse;
    
    size_t width = MagickGetImageWidth(letter);
    size_t height = MagickGetImageHeight(letter);
    
    if (width < 2 || height < 2)
    {
        PixelWand *white = NewPixelWand();
        PixelSetColor(white, "white");
        MagickNewImage(letter, 64, 64, white);
        DestroyPixelWand(white);
        return MagickTrue;
    }

    MagickResizeImage(letter, 64, 64, LanczosFilter);
    
    MagickSharpenImage(letter, 0, 0.8);
    
    return MagickTrue;
}

LetterExtractionResult split_word_into_letters(MagickWand *word_img,
                                                const char *debug_folder)
{
    LetterExtractionResult result = {false, NULL, 0};
    
    if (!word_img)
        return result;
    
    size_t width = MagickGetImageWidth(word_img);
    size_t height = MagickGetImageHeight(word_img);
    
    if (width < 5 || height < 5)
        return result;
    
    MagickWand *processed = CloneMagickWand(word_img);
    if (!processed)
        return result;
    
    MagickTransformImageColorspace(processed, GRAYColorspace);
    MagickNormalizeImage(processed);
    MagickThresholdImage(processed, 0.5 * QuantumRange);
    
    unsigned char *pixels = (unsigned char*)malloc(width * height);
    if (!pixels)
    {
        DestroyMagickWand(processed);
        return result;
    }
    
    MagickExportImagePixels(processed, 0, 0, width, height, "I",
                           CharPixel, pixels);
    DestroyMagickWand(processed);
    
    Region letter_regions[100];
    int letter_count = detect_letter_boundaries_by_gaps(pixels, width,
                                                        height,
                                                        letter_regions,
                                                        100);
    
    if (letter_count == 0)
    {
        free(pixels);
        return result;
    }
    
    int avg_width = 0;
    for (int i = 0; i < letter_count; i++)
    {
        avg_width += letter_regions[i].end - letter_regions[i].start;
    }
    avg_width /= letter_count;
    
    int min_letter_width = 1;
    
    letter_count = filter_noise_regions(letter_regions, letter_count,
                                        min_letter_width);
    
    if (letter_count == 0)
    {
        free(pixels);
        return result;
    }
    
    avg_width = 0;
    for (int i = 0; i < letter_count; i++)
    {
        avg_width += letter_regions[i].end - letter_regions[i].start;
    }
    avg_width /= letter_count;
    
    letter_count = split_wide_letters(pixels, width, height,
                                      letter_regions, letter_count,
                                      avg_width);
    
    if (letter_count == 0)
    {
        free(pixels);
        return result;
    }
    
    if (debug_folder && debug_folder[0] != '\0')
    {
        int *col_sum = (int*)calloc(width, sizeof(int));
        for (int x = 0; x < (int)width; x++)
        {
            for (int y = 0; y < (int)height; y++)
            {
                if (pixels[y * width + x] < 128)
                    col_sum[x]++;
            }
        }
        save_projection_visualization(col_sum, width, 1,
                                      debug_folder, "letter_cols", 1);
        free(col_sum);
    }
    
    result.letters = (MagickWand**)malloc(letter_count *
                                          sizeof(MagickWand*));
    if (!result.letters)
    {
        free(pixels);
        return result;
    }
    
    for (int i = 0; i < letter_count; i++)
    {
        int letter_x = letter_regions[i].start;
        int letter_width = letter_regions[i].end - letter_regions[i].start;
        
        int top = (int)height, bottom = 0;
        for (int y = 0; y < (int)height; y++)
        {
            int has_pixel = 0;
            for (int x = letter_x; x < letter_x + letter_width; x++)
            {
                if (x < (int)width && pixels[y * width + x] < 128)
                {
                    has_pixel = 1;
                    break;
                }
            }
            if (has_pixel)
            {
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
        
        int letter_height = bottom - top + 1;
        
        int padding = 2;
        letter_x = (letter_x > padding) ? (letter_x - padding) : 0;
        top = (top > padding) ? (top - padding) : 0;
        letter_width += 2 * padding;
        letter_height += 2 * padding;
        
        if (letter_x + letter_width > (int)width)
            letter_width = width - letter_x;
        if (top + letter_height > (int)height)
            letter_height = height - top;
        
        result.letters[i] = CloneMagickWand(word_img);
        if (!result.letters[i])
        {
            for (int j = 0; j < i; j++)
                DestroyMagickWand(result.letters[j]);
            free(result.letters);
            free(pixels);
            result.letters = NULL;
            return result;
        }
        
        if (MagickCropImage(result. letters[i], letter_width, letter_height,
                           letter_x, top) == MagickFalse)
        {
            for (int j = 0; j <= i; j++)
                DestroyMagickWand(result.letters[j]);
            free(result.letters);
            free(pixels);
            result.letters = NULL;
            return result;
        }
        
        MagickResetImagePage(result.letters[i], "");
        
        if (resize_letter_to_64x64(result.letters[i]) == MagickFalse)
        {
            fprintf(stderr, "[WARN] Failed to resize letter %d to 64x64\n", i);
        }
    }
    
    free(pixels);
    
    result.success = true;
    result.letter_count = letter_count;
    
    return result;
}

void free_letter_extraction_result(LetterExtractionResult *result)
{
    if (result && result->letters)
    {
        for (size_t i = 0; i < result->letter_count; i++)
        {
            if (result->letters[i])
                DestroyMagickWand(result->letters[i]);
        }
        free(result->letters);
        result->letters = NULL;
    }
}