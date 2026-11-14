#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static char debug_folder[1024] = "";

void set_word_extract_debug_folder(const char *folder)
{
    snprintf(debug_folder, sizeof(debug_folder), "%s", folder);
}

static int merge_nearby_regions(Region *regions, int count, int max_gap)
{
    if (count <= 1)
        return count;
    
    int write_idx = 0;
    regions[write_idx] = regions[0];
    
    for (int i = 1; i < count; i++)
    {
        int gap = regions[i].start - regions[write_idx].end;
        
        if (gap <= max_gap)
        {
            regions[write_idx].end = regions[i].end;
        }
        else
        {
            write_idx++;
            regions[write_idx] = regions[i];
        }
    }
    
    return write_idx + 1;
}

WordListBounds find_word_list_bounds(unsigned char *pixels, int width,
                                     int height, int grid_x, int grid_y,
                                     int grid_w, int grid_h)
{
    WordListBounds result = {false, 0, 0, 0, 0};
    
    for (int y = grid_y; y < grid_y + grid_h && y < height; y++)
    {
        for (int x = grid_x; x < grid_x + grid_w && x < width; x++)
        {
            pixels[y * width + x] = 255;
        }
    }
    
    typedef struct {
        int x, y, width, height;
        float score;
        char type[10];
    } Candidate;
    
    Candidate candidates[20];
    int candidate_count = 0;
    
    int *col_sum = (int*)calloc(width, sizeof(int));
    
    for (int x = 0; x < width; x++)
    {
        int sum = 0;
        for (int y = 0; y < height; y++)
        {
            if (pixels[y * width + x] < 128)
                sum++;
        }
        col_sum[x] = sum;
    }
    
    smooth_projection(col_sum, width, 15);
    
    int max_col_val = 0;
    for (int x = 0; x < width; x++)
    {
        if (col_sum[x] > max_col_val)
            max_col_val = col_sum[x];
    }
    
    int col_threshold = max_col_val * 0.2;
    
    Region col_regions[20];
    int col_region_count = 0;
    
    int in_region = 0;
    for (int x = 0; x < width && col_region_count < 20; x++)
    {
        if (col_sum[x] > col_threshold)
        {
            if (!in_region)
            {
                col_regions[col_region_count].start = x;
                in_region = 1;
            }
        }
        else if (in_region)
        {
            col_regions[col_region_count].end = x;
            col_region_count++;
            in_region = 0;
        }
    }
    
    if (in_region && col_region_count < 20)
    {
        col_regions[col_region_count].end = width;
        col_region_count++;
    }
    
    col_region_count = merge_nearby_regions(col_regions,
                                            col_region_count, 20);
    
    for (int i = 0; i < col_region_count && candidate_count < 20; i++)
    {
        int r_width = col_regions[i].end - col_regions[i].start;
        int r_start = col_regions[i].start;
        int r_end = col_regions[i].end;
        
        int overlap_start = (r_start > grid_x) ? r_start : grid_x;
        int overlap_end = (r_end < grid_x + grid_w) ?
                         r_end : (grid_x + grid_w);
        int overlap_width = (overlap_end > overlap_start) ?
                           (overlap_end - overlap_start) : 0;
        float overlap_ratio = (float)overlap_width / (float)r_width;
        
        if (overlap_ratio > 0.7f)
            continue;
        
        if (r_width < 40 || r_width > width * 0.6f)
            continue;
        
        int density = 0;
        for (int x = r_start; x < r_end && x < width; x++)
            density += col_sum[x];
        density /= r_width;
        
        int y_start = 0, y_end = height - 1;
        for (int y = 0; y < height; y++)
        {
            int row_count = 0;
            for (int x = r_start; x < r_end && x < width; x++)
            {
                if (pixels[y * width + x] < 128)
                    row_count++;
            }
            if (row_count > r_width * 0.03)
            {
                y_start = y;
                break;
            }
        }
        
        for (int y = height - 1; y >= 0; y--)
        {
            int row_count = 0;
            for (int x = r_start; x < r_end && x < width; x++)
            {
                if (pixels[y * width + x] < 128)
                    row_count++;
            }
            if (row_count > r_width * 0.03)
            {
                y_end = y;
                break;
            }
        }
        
        int region_height = y_end - y_start;
        
        if (region_height < height * 0.2f)
            continue;
        
        float score = (float)density * r_width;
        
        candidates[candidate_count].x = (r_start > 5) ?
                                        (r_start - 5) : 0;
        candidates[candidate_count].y = (y_start > 5) ?
                                        (y_start - 5) : 0;
        candidates[candidate_count].width = r_width + 10;
        candidates[candidate_count].height = region_height + 10;
        candidates[candidate_count].score = score;
        snprintf(candidates[candidate_count].type, 10, "%s", 
                 (r_start < grid_x) ? "left" : "right");
        
        candidate_count++;
    }
    
    free(col_sum);
    
    int *row_sum = (int*)calloc(height, sizeof(int));
    
    for (int y = 0; y < height; y++)
    {
        int sum = 0;
        for (int x = 0; x < width; x++)
        {
            if (pixels[y * width + x] < 128)
                sum++;
        }
        row_sum[y] = sum;
    }
    
    smooth_projection(row_sum, height, 5);
    
    int max_row_val = 0;
    for (int y = 0; y < height; y++)
    {
        if (row_sum[y] > max_row_val)
            max_row_val = row_sum[y];
    }
    
    int row_threshold = max_row_val * 0.15;
    
    Region row_regions[30];
    int row_region_count = 0;
    
    in_region = 0;
    for (int y = 0; y < height && row_region_count < 30; y++)
    {
        if (row_sum[y] > row_threshold)
        {
            if (!in_region)
            {
                row_regions[row_region_count].start = y;
                in_region = 1;
            }
        }
        else if (in_region)
        {
            row_regions[row_region_count].end = y;
            row_region_count++;
            in_region = 0;
        }
    }
    
    if (in_region && row_region_count < 30)
    {
        row_regions[row_region_count].end = height;
        row_region_count++;
    }
    
    row_region_count = merge_nearby_regions(row_regions,
                                            row_region_count, 15);
    
    for (int i = 0; i < row_region_count && candidate_count < 20; i++)
    {
        int r_height = row_regions[i].end - row_regions[i].start;
        int r_start = row_regions[i].start;
        int r_end = row_regions[i].end;
        
        int overlap_start = (r_start > grid_y) ? r_start : grid_y;
        int overlap_end = (r_end < grid_y + grid_h) ?
                         r_end : (grid_y + grid_h);
        int overlap_height = (overlap_end > overlap_start) ?
                            (overlap_end - overlap_start) : 0;
        float overlap_ratio = (float)overlap_height / (float)r_height;
        
        if (overlap_ratio > 0.7f)
            continue;
        
        if (r_height < 30)
            continue;
        
        int density = 0;
        for (int y = r_start; y < r_end && y < height; y++)
            density += row_sum[y];
        density /= r_height;
        
        int x_start = 0, x_end = width - 1;
        for (int x = 0; x < width; x++)
        {
            int col_count = 0;
            for (int y = r_start; y < r_end && y < height; y++)
            {
                if (pixels[y * width + x] < 128)
                    col_count++;
            }
            if (col_count > r_height * 0.03)
            {
                x_start = x;
                break;
            }
        }
        
        for (int x = width - 1; x >= 0; x--)
        {
            int col_count = 0;
            for (int y = r_start; y < r_end && y < height; y++)
            {
                if (pixels[y * width + x] < 128)
                    col_count++;
            }
            if (col_count > r_height * 0.03)
            {
                x_end = x;
                break;
            }
        }
        
        int region_width = x_end - x_start;
        
        if (region_width < width * 0.15f)
            continue;
        
        float score = (float)density * r_height * 1.5f;
        
        candidates[candidate_count].x = (x_start > 5) ?
                                        (x_start - 5) : 0;
        candidates[candidate_count].y = (r_start > 5) ?
                                        (r_start - 5) : 0;
        candidates[candidate_count].width = region_width + 10;
        candidates[candidate_count].height = r_height + 10;
        candidates[candidate_count].score = score;
        snprintf(candidates[candidate_count].type, 10, "%s",
                 (r_start < grid_y) ? "top" : "bottom");
        
        candidate_count++;
    }
    
    free(row_sum);
    
    int best_idx = -1;
    float best_score = 0;
    
    for (int i = 0; i < candidate_count; i++)
    {
        float score = candidates[i].score;
        
        if (strcmp(candidates[i].type, "top") == 0)
        {
            float aspect_ratio = (float)candidates[i].width /
                                (float)candidates[i].height;
            if (aspect_ratio > 4.0f)
                score *= 0.2f;
        }
        
        if (score > best_score)
        {
            best_score = score;
            best_idx = i;
        }
    }
    
    if (best_idx == -1)
        return result;
    
    result.success = true;
    result.x = candidates[best_idx].x;
    result.y = candidates[best_idx].y;
    result.width = candidates[best_idx].width;
    result.height = candidates[best_idx].height;
    
    return result;
}

WordListRegionResult extract_word_list_with_grid(MagickWand *original,
                                                  int grid_x, int grid_y,
                                                  int grid_w, int grid_h)
{
    WordListRegionResult result = {false, NULL, 0, 0, 0, 0};
    
    if (!original)
        return result;
    
    size_t width = MagickGetImageWidth(original);
    size_t height = MagickGetImageHeight(original);
    
    MagickWand *processed = CloneMagickWand(original);
    if (!processed)
        return result;
    
    MagickTransformImageColorspace(processed, GRAYColorspace);
    MagickNormalizeImage(processed);
    MagickThresholdImage(processed, 0.5 * QuantumRange);
    MagickDespeckleImage(processed);
    
    if (debug_folder[0] != '\0')
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s/word_detection/word_list_preprocessed.png",
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
    
    WordListBounds bounds = find_word_list_bounds(pixels, width, height,
                                                   grid_x, grid_y,
                                                   grid_w, grid_h);
    
    free(pixels);
    
    if (!bounds.success)
        return result;
    
    if (bounds.x + bounds.width > (int)width)
        bounds.width = width - bounds.x;
    if (bounds.y + bounds.height > (int)height)
        bounds.height = height - bounds.y;
    
    if (bounds.width < 30 || bounds.height < 30)
        return result;
    
    MagickWand *word_list_region = CloneMagickWand(original);
    if (!word_list_region)
        return result;
    
    if (MagickCropImage(word_list_region, bounds.width, bounds.height,
                       bounds.x, bounds.y) == MagickFalse)
    {
        DestroyMagickWand(word_list_region);
        return result;
    }
    
    MagickResetImagePage(word_list_region, "");
    
    if (debug_folder[0] != '\0')
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s/word_detection/word_list_region.png", debug_folder);
        MagickWriteImage(word_list_region, path);
    }
    
    result.success = true;
    result.word_list_region = word_list_region;
    result.x = bounds.x;
    result.y = bounds.y;
    result.width = bounds.width;
    result.height = bounds.height;
    
    return result;
}

void free_word_list_result(WordListRegionResult *result)
{
    if (result && result->word_list_region)
    {
        DestroyMagickWand(result->word_list_region);
        result->word_list_region = NULL;
    }
}