#include "grid_detect.h"
#include <stdio.h>

void compute_vertical_histogram(unsigned char *pixels, int width,
                                int height, int *hist)
{
    for (int x = 0; x < width; x++)
    {
        int count = 0;
        for (int y = 0; y < height; y++)
        {
            if (pixels[y * width + x] < 128)
                count++;
        }
        hist[x] = count;
    }
}

void compute_horizontal_histogram(unsigned char *pixels, int width,
                                  int height, int *hist)
{
    for (int y = 0; y < height; y++)
    {
        int count = 0;
        for (int x = 0; x < width; x++)
        {
            if (pixels[y * width + x] < 128)
                count++;
        }
        hist[y] = count;
    }
}

void print_histogram(const char *name, int *hist, int len, int threshold)
{
    (void)name;
    (void)hist;
    (void)len;
    (void)threshold;
}