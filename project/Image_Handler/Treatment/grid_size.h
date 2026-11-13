#ifndef GRID_SIZE_H
#define GRID_SIZE_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>

typedef struct
{
    bool success;
    size_t rows;
    size_t cols;
    double avg_cell_width;
    double avg_cell_height;
} GridSizeResult;

GridSizeResult detect_grid_size(MagickWand *cropped_grid);

void set_grid_size_debug_folder(const char *folder);

#endif
