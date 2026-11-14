#ifndef CELL_REPROCESS_H
#define CELL_REPROCESS_H

#include <MagickWand/MagickWand.h>
#include <stddef.h>

MagickBooleanType reprocess_cell(MagickWand *cell);
void reprocess_all_cells(MagickWand **cells, size_t count);

#endif