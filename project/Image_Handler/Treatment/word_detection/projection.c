#include "word_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void smooth_projection(int *proj, int len, int window)
{
    int *temp = (int*)malloc(len * sizeof(int));
    memcpy(temp, proj, len * sizeof(int));
    
    for (int i = 0; i < len; i++)
    {
        int sum = 0, count = 0;
        for (int j = i - window/2; j <= i + window/2; j++)
        {
            if (j >= 0 && j < len)
            {
                sum += temp[j];
                count++;
            }
        }
        proj[i] = sum / count;
    }
    
    free(temp);
}

void save_projection_visualization(int *projection, int len, 
                                   int threshold,
                                   const char *debug_folder,
                                   const char *name,
                                   int is_horizontal)
{
    if (!debug_folder || debug_folder[0] == '\0')
        return;
    
    MagickWand *wand = NewMagickWand();
    PixelWand *bg_color = NewPixelWand();
    PixelWand *bar_color = NewPixelWand();
    PixelWand *thresh_color = NewPixelWand();
    
    PixelSetColor(bg_color, "white");
    PixelSetColor(bar_color, "blue");
    PixelSetColor(thresh_color, "red");
    
    int vis_width = is_horizontal ? len : 600;
    int vis_height = is_horizontal ? 400 : len;
    
    MagickNewImage(wand, vis_width, vis_height, bg_color);
    
    int max_val = 0;
    for (int i = 0; i < len; i++)
    {
        if (projection[i] > max_val)
            max_val = projection[i];
    }
    
    if (max_val == 0)
        max_val = 1;
    
    DrawingWand *draw = NewDrawingWand();
    DrawSetFillColor(draw, bar_color);
    DrawSetStrokeColor(draw, bar_color);
    DrawSetStrokeWidth(draw, 1.0);
    
    if (is_horizontal)
    {
        for (int x = 0; x < len; x++)
        {
            int bar_height = (projection[x] * (vis_height - 40)) /
                            max_val;
            if (bar_height > 0)
            {
                DrawRectangle(draw, x, vis_height - bar_height - 20,
                             x + 1, vis_height - 20);
            }
        }
        
        DrawSetFillColor(draw, thresh_color);
        DrawSetStrokeColor(draw, thresh_color);
        int thresh_y = vis_height - 20 -
                      ((threshold * (vis_height - 40)) / max_val);
        DrawLine(draw, 0, thresh_y, vis_width, thresh_y);
    }
    else
    {
        for (int y = 0; y < len; y++)
        {
            int bar_width = (projection[y] * (vis_width - 40)) /
                           max_val;
            if (bar_width > 0)
            {
                DrawRectangle(draw, 20, y, 20 + bar_width, y + 1);
            }
        }
        
        DrawSetFillColor(draw, thresh_color);
        DrawSetStrokeColor(draw, thresh_color);
        int thresh_x = 20 + ((threshold * (vis_width - 40)) / max_val);
        DrawLine(draw, thresh_x, 0, thresh_x, vis_height);
    }
    
    MagickDrawImage(wand, draw);
    
    char filename[2048];
    snprintf(filename, sizeof(filename),
             "%s/word_detection/%s_projection.png", 
             debug_folder, name);
    MagickWriteImage(wand, filename);
    
    DestroyDrawingWand(draw);
    DestroyPixelWand(thresh_color);
    DestroyPixelWand(bar_color);
    DestroyPixelWand(bg_color);
    DestroyMagickWand(wand);
}