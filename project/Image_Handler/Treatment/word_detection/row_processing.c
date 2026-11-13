#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int detect_text_rows(int *row_sum, int height, int threshold,
                     Region *text_rows, int max_rows)
{
    int row_count = 0;
    int in_row = 0;
    
    for (int y = 0; y < height && row_count < max_rows; y++)
    {
        if (row_sum[y] > threshold)
        {
            if (!in_row)
            {
                text_rows[row_count].start = y;
                in_row = 1;
            }
        }
        else if (in_row)
        {
            text_rows[row_count].end = y;
            row_count++;
            in_row = 0;
        }
    }
    
    if (in_row && row_count < max_rows)
    {
        text_rows[row_count].end = height;
        row_count++;
    }
    
    return row_count;
}

int merge_close_rows(Region *text_rows, int row_count)
{
    if (row_count <= 1)
        return row_count;
    
    Region merged_rows[50];
    int merged_count = 0;
    merged_rows[merged_count] = text_rows[0];
    
    for (int i = 1; i < row_count; i++)
    {
        int gap = text_rows[i].start - merged_rows[merged_count].end;
        
        if (gap <= 2)
        {
            merged_rows[merged_count].end = text_rows[i].end;
        }
        else
        {
            merged_count++;
            merged_rows[merged_count] = text_rows[i];
        }
    }
    
    row_count = merged_count + 1;
    memcpy(text_rows, merged_rows, row_count * sizeof(Region));
    
    return row_count;
}

int calculate_median_height(Region *text_rows, int row_count)
{
    if (row_count == 0)
        return 20;
    
    int heights[50];
    for (int i = 0; i < row_count; i++)
    {
        heights[i] = text_rows[i].end - text_rows[i].start;
    }
    
    for (int i = 0; i < row_count - 1; i++)
    {
        for (int j = 0; j < row_count - i - 1; j++)
        {
            if (heights[j] > heights[j + 1])
            {
                int temp = heights[j];
                heights[j] = heights[j + 1];
                heights[j + 1] = temp;
            }
        }
    }
    
    return heights[row_count / 2];
}