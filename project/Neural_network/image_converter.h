#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

// Constants
// Image dimensions
#define WIDTH 64
#define HEIGHT 64
#define PIXELS (WIDTH * HEIGHT)

float *load_pgm_as_vector(const char *filename);

#endif


