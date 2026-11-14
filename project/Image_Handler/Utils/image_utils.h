#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <MagickWand/MagickWand.h>
#include <stdbool.h>

bool save_magickwand_as_png(MagickWand *wand, const char *output_path);

bool save_magickwand_as(MagickWand *wand, const char *output_path, 
                        const char *format);

#endif