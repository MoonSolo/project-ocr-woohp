#include "preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ROTATION_ANGLE_THRESHOLD 0.7   
#define DESKEW_THRESHOLD         40.0  

#define MIN_ROTATION_ANGLE      -45.0
#define MAX_ROTATION_ANGLE       45.0

static char rotation_debug_folder[1024] = "";

void set_rotation_debug_folder(const char *folder)
{
    if (folder)
        snprintf(rotation_debug_folder, sizeof(rotation_debug_folder), "%s", folder);
    else
        rotation_debug_folder[0] = '\0';
}

static void save_rotation_debug_image(MagickWand *wand, const char *filename)
{
    if (rotation_debug_folder[0] != '\0' && wand && filename)
    {
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", rotation_debug_folder, filename);
        MagickWriteImage(wand, path);
        printf("[DEBUG] Saved rotation debug image: %s\n", path);
    }
}

static MagickWand* deskew_with_imagemagick(MagickWand *input, double threshold_percent)
{
    if (!input)
    {
        printf("deskew_with_imagemagick: Input is NULL\n");
        return NULL;
    }

    MagickWand *deskewed = CloneMagickWand(input);
    if (!deskewed)
    {
        printf("deskew_with_imagemagick:  Failed to clone input\n");
        return NULL;
    }

    double threshold = (threshold_percent / 100.0) * QuantumRange;
    MagickTransformImageColorspace(deskewed, GRAYColorspace);
    MagickNormalizeImage(deskewed);

    if (MagickDeskewImage(deskewed, threshold) == MagickFalse)
    {
        printf("deskew_with_imagemagick: MagickDeskewImage failed\n");
        DestroyMagickWand(deskewed);
        return NULL;
    }

    save_rotation_debug_image(deskewed, "rotation_corrected.png");
    return deskewed;
}

MagickWand* rotate_image(MagickWand *input, double angle_degrees)
{
    if (!input)
    {
        printf("rotate_image:  Input image is NULL\n");
        return NULL;
    }

    MagickWand *rotated = CloneMagickWand(input);
    if (!rotated)
    {
        printf("rotate_image: Failed to clone input\n");
        return NULL;
    }

    PixelWand *background = NewPixelWand();
    if (!background)
    {
        printf("rotate_image: Failed to create background PixelWand\n");
        DestroyMagickWand(rotated);
        return NULL;
    }

    PixelSetColor(background, "white");

    if (MagickRotateImage(rotated, background, angle_degrees) == MagickFalse)
    {
        printf("rotate_image:  MagickRotateImage failed\n");
        DestroyPixelWand(background);
        DestroyMagickWand(rotated);
        return NULL;
    }

    MagickResetImagePage(rotated, "");
    DestroyPixelWand(background);

    printf("[INFO] Rotated image by %.2f degrees\n", angle_degrees);
    return rotated;
}

MagickWand* auto_rotate_image(MagickWand *input)
{
    if (!input)
    {
        printf("auto_rotate_image: Input image is NULL\n");
        return NULL;
    }

    printf("[INFO] Starting automatic rotation correction (ImageMagick deskew)...\n");

    MagickWand *deskewed = deskew_with_imagemagick(input, DESKEW_THRESHOLD);
    if (!deskewed)
    {
        printf("[WARNING] Deskew did not succeed; returning clone of original\n");
        MagickWand *clone = CloneMagickWand(input);
        if (!clone)
            printf("auto_rotate_image: Failed to clone original on fallback\n");
        return clone;
    }

    printf("[INFO] Automatic rotation correction (deskew) completed\n");
    return deskewed;
}