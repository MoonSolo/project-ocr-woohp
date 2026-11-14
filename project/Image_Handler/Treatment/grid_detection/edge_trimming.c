#include "grid_detect.h"
#include <stdio.h>

Rectangle trim_edge_decorations(Rectangle region, int *h_hist, int *v_hist,
                               int width, int height, int avg_density)
{
    Rectangle trimmed = region;
    
    long long sum = 0;
    int count = 0;
    
    for (int y = region.y; y < region.y + region.height && y < height; y++)
    {
        sum += h_hist[y];
        count++;
    }
    
    int mean_density = (int)(sum / count);
    
    int top_edge_size = region.height * 0.3;
    int top_extreme_count = 0;
    int top_extreme_end = -1;
    
    for (int y = region.y; y < region.y + top_edge_size && y < height; y++)
    {
        if (h_hist[y] > EXTREME_THRESHOLD)
        {
            top_extreme_end = y;
            top_extreme_count++;
        }
    }
    
    if (top_extreme_count >= 5 && top_extreme_end > region.y)
    {
        int new_top = top_extreme_end + 1;
        
        while (new_top < region.y + region.height &&
               h_hist[new_top] < mean_density * 0.2)
        {
            new_top++;
        }
        
        int trimmed_pixels = new_top - region.y;
        if (trimmed_pixels > 10 && trimmed_pixels < region.height * 0.4)
        {
            trimmed.y = new_top;
            trimmed.height -= trimmed_pixels;
        }
    }
    
    int bottom_edge_start = trimmed.y + trimmed.height -
                           (int)(trimmed.height * 0.3);
    int bottom_extreme_count = 0;
    int bottom_extreme_start = -1;
    int bottom_max_density = 0;
    
    for (int y = bottom_edge_start;
         y < trimmed.y + trimmed.height && y < height; y++)
    {
        if (h_hist[y] > EXTREME_THRESHOLD)
        {
            if (bottom_extreme_start == -1)
                bottom_extreme_start = y;
            bottom_extreme_count++;
            if (h_hist[y] > bottom_max_density)
                bottom_max_density = h_hist[y];
        }
    }
    
    if (bottom_extreme_count >= 4 &&
        bottom_extreme_start < trimmed.y + trimmed.height)
    {
        int gap_rows = 0;
        int search_start = bottom_extreme_start - 20;
        if (search_start < trimmed.y)
            search_start = trimmed.y;
        
        for (int y = search_start; y < bottom_extreme_start; y++)
        {
            if (h_hist[y] < mean_density * 0.3)
                gap_rows++;
        }
        
        int is_very_extreme = (bottom_max_density > VERY_EXTREME_THRESHOLD);
        
        int should_trim = (gap_rows >= 2) || is_very_extreme;
        
        if (should_trim)
        {
            int new_bottom = bottom_extreme_start;
            int normal_streak = 0;
            
            for (int y = bottom_extreme_start - 1; y > search_start; y--)
            {
                int density = h_hist[y];
                
                if (density >= mean_density * 0.4 &&
                    density <= EXTREME_THRESHOLD)
                {
                    normal_streak++;
                    new_bottom = y + 1;
                    
                    if (normal_streak >= 3)
                        break;
                }
                else
                {
                    normal_streak = 0;
                }
            }
            
            if (normal_streak < 3)
                new_bottom = bottom_extreme_start;
            
            int trimmed_pixels = (trimmed.y + trimmed.height) - new_bottom;
            
            if (trimmed_pixels > 10 &&
                trimmed_pixels < trimmed.height * 0.5)
            {
                trimmed.height = new_bottom - trimmed.y;
            }
        }
    }
    
    int left_edge_size = (int)(trimmed.width * 0.2);
    int left_extreme_count = 0;
    int left_extreme_end = -1;
    int left_max_density = 0;
    
    for (int x = trimmed.x; x < trimmed.x + left_edge_size && x < width; x++)
    {
        if (v_hist[x] > EXTREME_THRESHOLD)
        {
            left_extreme_end = x;
            left_extreme_count++;
            if (v_hist[x] > left_max_density)
                left_max_density = v_hist[x];
        }
    }
    
    if (left_extreme_count >= 4 && left_extreme_end > trimmed.x)
    {
        int is_very_extreme = (left_max_density > VERY_EXTREME_THRESHOLD);
        
        if (is_very_extreme || left_extreme_count >= 6)
        {
            int new_left = left_extreme_end + 1;
            
            while (new_left < trimmed.x + trimmed.width &&
                   v_hist[new_left] < mean_density * 0.2)
            {
                new_left++;
            }
            
            int trimmed_pixels = new_left - trimmed.x;
            
            if (trimmed_pixels > 10 &&
                trimmed_pixels < trimmed.width * 0.3)
            {
                trimmed.x = new_left;
                trimmed.width -= trimmed_pixels;
            }
        }
    }
    
    int right_edge_start = trimmed.x + trimmed.width -
                          (int)(trimmed.width * 0.2);
    int right_extreme_count = 0;
    int right_extreme_start = -1;
    int right_max_density = 0;
    
    for (int x = right_edge_start;
         x < trimmed.x + trimmed.width && x < width; x++)
    {
        if (v_hist[x] > EXTREME_THRESHOLD)
        {
            if (right_extreme_start == -1)
                right_extreme_start = x;
            right_extreme_count++;
            if (v_hist[x] > right_max_density)
                right_max_density = v_hist[x];
        }
    }
    
    if (right_extreme_count >= 4 &&
        right_extreme_start < trimmed.x + trimmed.width)
    {
        int is_very_extreme = (right_max_density > VERY_EXTREME_THRESHOLD);
        
        if (is_very_extreme || right_extreme_count >= 6)
        {
            int new_right = right_extreme_start;
            int normal_streak = 0;
            
            for (int x = right_extreme_start - 1; x > trimmed.x; x--)
            {
                int density = v_hist[x];
                
                if (density >= mean_density * 0.4 &&
                    density <= EXTREME_THRESHOLD)
                {
                    normal_streak++;
                    new_right = x + 1;
                    
                    if (normal_streak >= 3)
                        break;
                }
                else
                {
                    normal_streak = 0;
                }
            }
            
            if (normal_streak < 3)
                new_right = right_extreme_start;
            
            int trimmed_pixels = (trimmed.x + trimmed.width) - new_right;
            
            if (trimmed_pixels > 10 &&
                trimmed_pixels < trimmed.width * 0.3)
            {
                trimmed.width = new_right - trimmed.x;
            }
        }
    }
    
    trimmed.area = trimmed.width * trimmed.height;
    
    return trimmed;
}