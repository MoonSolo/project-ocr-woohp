#ifndef CELL_EXTRACT_H
#define CELL_EXTRACT_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>

typedef struct
{
    bool success;
    MagickWand **cells;
    size_t rows;
    size_t cols;
    size_t total_cells;
} CellExtractionResult;

CellExtractionResult extract_cells(MagickWand *cropped_grid,
                                   size_t rows, size_t cols,
                                   double avg_cell_width,
                                   double avg_cell_height);

void free_cell_extraction_result(CellExtractionResult *result);

void set_cell_extract_debug_folder(const char *folder);

#endif
