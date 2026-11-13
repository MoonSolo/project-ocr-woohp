#ifndef GRID_DETECT_H
#define GRID_DETECT_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>

typedef struct
{
    bool detected;
    int x, y, width, height;
} GridDetectionResult;

typedef struct
{
    int x, y, width, height;
    int area;
    float density;
} Rectangle;

void compute_vertical_histogram(unsigned char *pixels, int width,
                                int height, int *hist);

void compute_horizontal_histogram(unsigned char *pixels, int width,
                                  int height, int *hist);

void print_histogram(const char *name, int *hist, int len, int threshold);

Rectangle extract_bounds(unsigned char *pixels, int width, int height,
                        int x_min, int x_max, int y_min, int y_max);

float score_as_grid(Rectangle rect, int image_width, int image_height);

Rectangle trim_edge_decorations(Rectangle region, int *h_hist, int *v_hist,
                               int width, int height, int avg_density);

GridDetectionResult detect_grid(MagickWand *input);

void set_grid_detection_debug_folder(const char *folder);

#define MIN_RECT_SIZE 100
#define MAX_RECTANGLES 30
#define MARGIN 5
#define MIN_SIGNIFICANT_GAP 15
#define EXTREME_THRESHOLD 120
#define VERY_EXTREME_THRESHOLD 200

#endif
