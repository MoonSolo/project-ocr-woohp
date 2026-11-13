#include "preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float threshold_lvl = 0.5;
float noise_reduc_rad = 1.0;

static char debug_folder[1024] = "";

void set_preprocess_debug_folder(const char *folder)
{
    if (folder)
        snprintf(debug_folder, sizeof(debug_folder), "%s", folder);
    else
        debug_folder[0] = '\0';
}

static void save_debug_image(MagickWand *wand, const char *filename)
{
    if (debug_folder[0] != '\0' && wand && filename)
    {
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", debug_folder, filename);
        MagickWriteImage(wand, path);
    }
}

static MagickBooleanType convert_to_grayscale(MagickWand *wand)
{
    if (!wand) return MagickFalse;

    MagickBooleanType result = MagickTransformImageColorspace(wand, GRAYColorspace);

    if (result == MagickTrue)
        save_debug_image(wand, "01_grayscale.png");

    return result;
}

static MagickBooleanType normalize_contrast(MagickWand *wand)
{
    if (!wand) return MagickFalse;

    if (MagickNormalizeImage(wand) == MagickFalse)
        return MagickFalse;

    if (MagickContrastStretchImage(wand, 0.05 * QuantumRange,
            0.95 * QuantumRange) == MagickFalse)
        return MagickFalse;

    save_debug_image(wand, "02_normalized.png");

    return MagickTrue;
}

static MagickBooleanType apply_threshold(MagickWand *wand)
{
    if (!wand) return MagickFalse;

    MagickBooleanType result = MagickAdaptiveThresholdImage(wand, 31, 31, -8);

    if (result == MagickTrue)
        save_debug_image(wand, "03_thresholded.png");

    return result;
}

static MagickBooleanType reduce_noise(MagickWand *wand)
{
    if (!wand) return MagickFalse;

    MagickBooleanType result = MagickDespeckleImage(wand);

    if (result == MagickTrue)
        save_debug_image(wand, "04_noise_reduced.png");

    return result;
}

MagickWand* preprocess_image(MagickWand *input)
{
    if(!input)
    {
        printf("preprocess_image: Input image is NULL\n");
        return NULL;
    }

    MagickWand *processed = CloneMagickWand(input);
    if(!processed)
    {
        printf("preprocess_image: Failed to clone input image\n");
        return NULL;
    }

    if(convert_to_grayscale(processed) == MagickFalse)
    {
        printf("preprocess_image: Failed to convert image to grayscale\n");
        DestroyMagickWand(processed);
        return NULL;
    }

    if(normalize_contrast(processed) == MagickFalse)
    {
        printf("preprocess_image: Failed to normalize the contrast of the image\n");
        DestroyMagickWand(processed);
        return NULL;
    }

    if(apply_threshold(processed) == MagickFalse)
    {
        printf("preprocess_image: Failed to apply threshold to the image\n");
        DestroyMagickWand(processed);
        return NULL;
    }

    if (reduce_noise(processed) == MagickFalse)
    {
        printf("preprocess_image: Failed to reduce noise of the image\n");
        DestroyMagickWand(processed);
        return NULL;
    }

    return processed;
}



