#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>
#include <stddef.h>

extern float threshold_lvl;
extern float noise_reduc_rad;

typedef struct 
{
    MagickWand *wand;
    char error[512];
    ExceptionType severity;
} LoadImageResult;

void imagemagick_init(void);
void imagemagick_terminate(void);

LoadImageResult load_image(const char *path);
MagickWand* preprocess_image(MagickWand *input);

void set_preprocess_debug_folder(const char *folder);

void set_rotation_debug_folder(const char *folder);
MagickWand* auto_rotate_image(MagickWand *input);
MagickWand* rotate_image(MagickWand *input, double angle_degrees);

#endif
