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

static int has_noisy_background(MagickWand *wand)
{
    if (! wand) return 0;
    
    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);
    
    int sample_w = width / 5;
    int sample_h = height / 5;
    
    unsigned char *pixels = (unsigned char*)malloc(sample_w * sample_h);
    if (!pixels) return 0;
    
    MagickExportImagePixels(wand, width / 10, 0, sample_w, sample_h,
                           "I", CharPixel, pixels);
    
    long long sum = 0;
    for (int i = 0; i < sample_w * sample_h; i++)
        sum += pixels[i];
    
    int mean = sum / (sample_w * sample_h);
    
    long long variance_sum = 0;
    for (int i = 0; i < sample_w * sample_h; i++)
    {
        int diff = pixels[i] - mean;
        variance_sum += diff * diff;
    }
    
    int variance = variance_sum / (sample_w * sample_h);
    
    free(pixels);
    
    printf("[DEBUG] Background variance: %d (mean: %d)\n", variance, mean);
    
    int is_textured = (variance > 400 && mean > 200);
    
    if (is_textured)
        printf("[DEBUG] → Classified as TEXTURED background\n");
    else
        printf("[DEBUG] → Classified as CLEAN background\n");
    
    return is_textured;
}

static MagickBooleanType remove_background_texture(MagickWand *wand)
{
    if (! wand) return MagickFalse;
    
    printf("[INFO] Removing background texture via subtraction.. .\n");
    
    MagickWand *background = CloneMagickWand(wand);
    if (!background) return MagickFalse;
    
    MagickGaussianBlurImage(background, 0, 15.0);  
    save_debug_image(background, "02a_background_estimate.png");
    
    MagickNegateImage(background, MagickFalse);
    MagickCompositeImage(wand, background, PlusCompositeOp, MagickTrue, 0, 0);  
    
    DestroyMagickWand(background);
    save_debug_image(wand, "02b_after_subtraction.png");
    
    MagickNormalizeImage(wand);
    MagickContrastStretchImage(wand, 0.05 * QuantumRange, 0.95 * QuantumRange);
    save_debug_image(wand, "02c_normalized.png");
    
    MagickThresholdImage(wand, 0.5 * QuantumRange);
    save_debug_image(wand, "02d_thresholded.png");
    
    MagickDespeckleImage(wand);
    save_debug_image(wand, "02e_cleaned.png");
    
    printf("[INFO] Background texture removed\n");
    return MagickTrue;
}

MagickWand* preprocess_image(MagickWand *input)
{
    if (!input)
    {
        printf("preprocess_image: Input image is NULL\n");
        return NULL;
    }

    MagickWand *processed = CloneMagickWand(input);
    if (!processed)
    {
        printf("preprocess_image: Failed to clone input image\n");
        return NULL;
    }

    if (convert_to_grayscale(processed) == MagickFalse)
    {
        printf("preprocess_image: Failed to convert image to grayscale\n");
        DestroyMagickWand(processed);
        return NULL;
    }

    int is_textured = has_noisy_background(processed);

    if (is_textured)
    {
        if (remove_background_texture(processed) == MagickFalse)
        {
            printf("preprocess_image:  Background texture removal failed\n");
            DestroyMagickWand(processed);
            return NULL;
        }
    }
    else
    {
        if (normalize_contrast(processed) == MagickFalse)
        {
            printf("preprocess_image: Failed to normalize the contrast of the image\n");
            DestroyMagickWand(processed);
            return NULL;
        }

        if (apply_threshold(processed) == MagickFalse)
        {
            printf("preprocess_image: Failed to apply threshold to the image\n");
            DestroyMagickWand(processed);
            return NULL;
        }

        if (reduce_noise(processed) == MagickFalse)
        {
            printf("preprocess_image:  Failed to reduce noise of the image\n");
            DestroyMagickWand(processed);
            return NULL;
        }
    }

    return processed;
}



