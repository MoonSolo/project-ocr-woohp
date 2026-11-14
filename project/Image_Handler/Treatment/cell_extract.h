#ifndef CELL_EXTRACT_H
#define CELL_EXTRACT_H

#include <MagickWand/MagickWand.h>

typedef struct
{
    int success;
    MagickWand **cells;
    size_t rows;
    size_t cols;
    size_t total_cells;
} CellExtractionResult;

void set_cell_extraction_debug_folder(const char *folder);
CellExtractionResult extract_cells(MagickWand *grid_image, int rows, int cols);
void free_cells(CellExtractionResult *result);

#endif
