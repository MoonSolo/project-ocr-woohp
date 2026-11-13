#include "image_converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float *load_pgm_as_vector(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        perror("Error opening file");
        return NULL;
    }

    char line[256];
    int width = 0, height = 0, maxval = 0;

    // Read magic number
    if (!fgets(line, sizeof(line), fp) || strncmp(line, "P5", 2) != 0)
    {
        fprintf(stderr, "Not a valid P5 PGM file: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    // Read lines until we have width, height
    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#')
            continue; 

        if (sscanf(line, "%d %d", &width, &height) == 2)
            break; 
    }

    if (width != WIDTH || height != HEIGHT)
    {
        fprintf(stderr, "Expected %dx%d image, got %dx%d in %s\n", WIDTH, HEIGHT, width, height, filename);
        fclose(fp);
        return NULL;
    }

    // Read maxval line
    do
    {
        if (!fgets(line, sizeof(line), fp))
        {
            fprintf(stderr, "Missing maxval line in %s\n", filename);
            fclose(fp);
            return NULL;
        }
    } while (line[0] == '#');

    if (sscanf(line, "%d", &maxval) != 1 || maxval != 255)
    {
        fprintf(stderr, "Invalid maxval (expected 255) in %s\n", filename);
        fclose(fp);
        return NULL;
    }

    // Read data
    float *vector = malloc(sizeof(float) * PIXELS);
    if (!vector)
    {
        perror("malloc vector");
        fclose(fp);
        return NULL;
    }

    unsigned char *pixels = malloc(PIXELS);
    if (!pixels)
    {
        perror("malloc pixels");
        free(vector);
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(pixels, 1, PIXELS, fp);
    fclose(fp);

    if (read_bytes != PIXELS)
    {
        fprintf(stderr, "Error: Expected %d bytes, got %zu in %s\n", PIXELS, read_bytes, filename);
        free(pixels);
        free(vector);
        return NULL;
    }

    // Normalize values
    for (int i = 0; i < PIXELS; i++)
        vector[i] = pixels[i] / 255.0f;

    free(pixels);
    return vector;
}


