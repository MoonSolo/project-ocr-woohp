#include "word_detection.h"
#include <string.h>

int merge_word_regions(Region *words_in_row, int words_in_row_count)
{
    if (words_in_row_count <= 1)
        return words_in_row_count;
    
    int gaps[50];
    int gap_count = 0;
    
    for (int i = 0; i < words_in_row_count - 1; i++)
    {
        gaps[gap_count++] = words_in_row[i + 1].start -
                           words_in_row[i].end;
    }
    
    for (int i = 0; i < gap_count - 1; i++)
    {
        for (int j = 0; j < gap_count - i - 1; j++)
        {
            if (gaps[j] > gaps[j + 1])
            {
                int temp = gaps[j];
                gaps[j] = gaps[j + 1];
                gaps[j + 1] = temp;
            }
        }
    }
    
    int median_gap = gaps[gap_count / 2];
    int merge_threshold = median_gap / 2;
    if (merge_threshold < 3) merge_threshold = 3;
    if (merge_threshold > 8) merge_threshold = 8;
    
    int merged_count = 0;
    Region merged_words[50];
    merged_words[merged_count] = words_in_row[0];
    
    for (int i = 1; i < words_in_row_count; i++)
    {
        int gap = words_in_row[i].start - merged_words[merged_count].end;
        
        if (gap <= merge_threshold)
        {
            merged_words[merged_count].end = words_in_row[i].end;
        }
        else
        {
            merged_count++;
            merged_words[merged_count] = words_in_row[i];
        }
    }
    
    words_in_row_count = merged_count + 1;
    memcpy(words_in_row, merged_words,
           words_in_row_count * sizeof(Region));
    
    return words_in_row_count;
}