#ifndef WORD_DETECTION_H
#define WORD_DETECTION_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    bool success;
    int x, y, width, height;
} WordListBounds;

typedef struct
{
    bool success;
    MagickWand *word_list_region;
    int x, y, width, height;
} WordListRegionResult;

typedef struct
{
    bool success;
    MagickWand **words;
    size_t word_count;
} WordExtractionResult;

typedef struct
{
    bool success;
    MagickWand **letters;
    size_t letter_count;
} LetterExtractionResult;

typedef struct { 
    int start, end; 
} Region;

typedef struct {
    int x, y, width, height;
} WordBounds;

void set_word_extract_debug_folder(const char *folder);

WordListRegionResult extract_word_list_with_grid(MagickWand *original,
                                                  int grid_x, int grid_y,
                                                  int grid_w, int grid_h);

WordExtractionResult extract_individual_words(
    MagickWand *word_list_region, 
    const char *debug_folder);

LetterExtractionResult split_word_into_letters(MagickWand *word_img,
                                                const char *debug_folder);

void free_word_list_result(WordListRegionResult *result);
void free_word_extraction_result(WordExtractionResult *result);
void free_letter_extraction_result(LetterExtractionResult *result);

void smooth_projection(int *proj, int len, int window);

void save_projection_visualization(int *projection, int len, 
                                   int threshold,
                                   const char *debug_folder,
                                   const char *name,
                                   int is_horizontal);

int detect_text_rows(int *row_sum, int height, int threshold,
                     Region *text_rows, int max_rows);

int merge_close_rows(Region *text_rows, int row_count);

int calculate_median_height(Region *text_rows, int row_count);

int merge_word_regions(Region *words_in_row, int words_in_row_count);

int process_tall_row_by_columns(unsigned char *pixels, int width,
                                int height, Region tall_row,
                                int median_height,
                                WordBounds *word_bounds,
                                int *word_count, int max_words);

#endif
