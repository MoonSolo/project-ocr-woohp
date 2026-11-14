#include "cell_reprocess.h"
#include <stdio.h>

MagickBooleanType reprocess_cell(MagickWand *cell)
{
    if (!cell) return MagickFalse;
    
    size_t orig_width = MagickGetImageWidth(cell);
    size_t orig_height = MagickGetImageHeight(cell);
    
    printf("[DEBUG] Cell size before processing: %zux%zu\n", orig_width, orig_height);
    
    if (orig_width < 2 || orig_height < 2)
    {
        printf("[DEBUG] Cell too small, creating blank\n");
        PixelWand *white = NewPixelWand();
        PixelSetColor(white, "white");
        MagickNewImage(cell, 64, 64, white);
        DestroyPixelWand(white);
        return MagickTrue;
    }
    
    MagickResizeImage(cell, 64, 64, LanczosFilter);
    
    MagickSharpenImage(cell, 0, 0.8);
    
    printf("[DEBUG] Cell resized to 64x64 with light sharpening\n");
    
    return MagickTrue;
}

void reprocess_all_cells(MagickWand **cells, size_t count)
{
    if (!cells) return;
    
    printf("[INFO] Reprocessing %zu cells to 64x64.. .\n", count);
    
    for (size_t i = 0; i < count; i++)
    {
        if (cells[i])
        {
            printf("[DEBUG] Processing cell %zu/%zu\n", i + 1, count);
            reprocess_cell(cells[i]);
        }
        else
        {
            printf("[DEBUG] Cell %zu is NULL\n", i + 1);
        }
    }
    
    printf("[INFO] ✓ Reprocessed %zu cells\n", count);
}