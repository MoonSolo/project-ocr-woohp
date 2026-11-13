#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int process_tall_row_by_columns(unsigned char *pixels, int width,
                                int height, Region tall_row,
                                int median_height,
                                WordBounds *word_bounds,
                                int *word_count, int max_words)
{
    int row_start = tall_row.start;
    int row_end = tall_row.end;
    int row_height = row_end - row_start;
    int col_width = width / 3;
    
    for (int col = 0; col < 3 && *word_count < max_words; col++)
    {
        int col_start = col * col_width;
        int col_end = (col == 2) ? width : (col + 1) * col_width;
        
        int *col_row_sum = (int*)calloc(row_height, sizeof(int));
        
        for (int y = 0; y < row_height; y++)
        {
            int sum = 0;
            for (int x = col_start; x < col_end && x < width; x++)
            {
                int img_y = row_start + y;
                if (img_y < height && pixels[img_y * width + x] < 128)
                    sum++;
            }
            col_row_sum[y] = sum;
        }
        
        int text_threshold = (col_end - col_start) * 0.10;
        
        Region col_text_rows[20];
        int col_row_count = 0;
        int in_text = 0;
        int text_start = 0;
        
        for (int y = 0; y < row_height && col_row_count < 20; y++)
        {
            if (col_row_sum[y] > text_threshold)
            {
                if (!in_text)
                {
                    text_start = y;
                    in_text = 1;
                }
            }
            else if (in_text)
            {
                int h = y - text_start;
                if (h >= median_height / 2 && h <= median_height * 3)
                {
                    col_text_rows[col_row_count].start = row_start +
                                                         text_start;
                    col_text_rows[col_row_count].end = row_start + y;
                    col_row_count++;
                }
                in_text = 0;
            }
        }
        
        if (in_text && col_row_count < 20)
        {
            int h = row_height - text_start;
            if (h >= median_height / 2 && h <= median_height * 3)
            {
                col_text_rows[col_row_count].start = row_start +
                                                     text_start;
                col_text_rows[col_row_count].end = row_end;
                col_row_count++;
            }
        }
        
        free(col_row_sum);
        
        for (int r = 0; r < col_row_count && *word_count < max_words; r++)
        {
            int text_row_start = col_text_rows[r].start;
            int text_row_end = col_text_rows[r].end;
            int text_row_height = text_row_end - text_row_start;
            
            int *word_col_sum = (int*)calloc(col_end - col_start,
                                            sizeof(int));
            
            for (int x = 0; x < (col_end - col_start); x++)
            {
                int sum = 0;
                for (int y = text_row_start;
                     y < text_row_end && y < height; y++)
                {
                    int real_x = col_start + x;
                    if (real_x < width &&
                        pixels[y * width + real_x] < 128)
                        sum++;
                }
                word_col_sum[x] = sum;
            }
            
            int word_threshold = text_row_height * 0.05;
            
            Region words_in_row[20];
            int words_in_row_count = 0;
            int in_word = 0;
            
            for (int x = 0;
                 x <= (col_end - col_start) && words_in_row_count < 20;
                 x++)
            {
                int has_text = (x < (col_end - col_start)) &&
                              (word_col_sum[x] > word_threshold);
                
                if (has_text)
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
            
            words_in_row_count = merge_word_regions(words_in_row,
                                                    words_in_row_count);
            
            for (int w = 0;
                 w < words_in_row_count && *word_count < max_words; w++)
            {
                int word_width = words_in_row[w].end -
                                words_in_row[w].start;
                float aspect = (float)word_width /
                              (float)text_row_height;
                
                if (word_width < 15)
                    continue;
                
                if (word_width > (col_end - col_start) * 0.85f)
                    continue;
                
                if (aspect < 1.5f && word_width > 80)
                    continue;
                
                word_bounds[*word_count].x = col_start +
                                             words_in_row[w].start;
                word_bounds[*word_count].y = (text_row_start > 2) ?
                                             text_row_start - 2 :
                                             text_row_start;
                word_bounds[*word_count].width = word_width + 4;
                word_bounds[*word_count].height = text_row_height + 4;
                
                if (word_bounds[*word_count].x +
                    word_bounds[*word_count].width > width)
                    word_bounds[*word_count].width = width -
                                                     word_bounds[*word_count].x;
                if (word_bounds[*word_count].y +
                    word_bounds[*word_count].height > height)
                    word_bounds[*word_count].height = height -
                                                      word_bounds[*word_count].y;
                
                (*word_count)++;
            }
            
            free(word_col_sum);
        }
    }
    
    return 0;
}