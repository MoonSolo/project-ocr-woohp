#include "image_utils.h"
#include <stdio.h>
#include <string.h>

bool save_magickwand_as_png(MagickWand *wand, const char *output_path)
{
    if (!wand) 
    {
        printf("save_magickwand_as_png: Wand is NULL\n");
        return false;
    }

    if (!output_path)
    {
        printf("save_magickwand_as_png: Output path is NULL\n");
        return false;
    }

    if (MagickSetImageFormat(wand, "PNG") == MagickFalse) 
    {
        printf("save_magickwand_as_png: Failed to set image format to PNG\n");
        return false;
    }

    if (MagickWriteImage(wand, output_path) == MagickFalse) 
    {
        ExceptionType severity;
        char *desc = MagickGetException(wand, &severity);

        printf("save_magickwand_as_png: Failed to write '%s': %s\n",
                output_path, desc ? desc : "Unknown error");

        if (desc) MagickRelinquishMemory(desc);

        return false;
    }

    printf("[DEBUG] Saved image to '%s'\n", output_path);
    return true;
}

bool save_magickwand_as(MagickWand *wand, const char *output_path, 
                        const char *format)
{
    if (!wand) 
    {
        printf("save_magickwand_as: Wand is NULL\n");
        return false;
    }

    if (!output_path)
    {
        printf("save_magickwand_as: Output path is NULL\n");
        return false;
    }

    if (!format)
    {
        printf("save_magickwand_as: Format is NULL\n");
        return false;
    }

    if (MagickSetImageFormat(wand, format) == MagickFalse) 
    {
        printf("save_magickwand_as: Failed to set image format to %s\n", format);
        return false;
    }

    if (MagickWriteImage(wand, output_path) == MagickFalse) 
    {
        ExceptionType severity;
        char *desc = MagickGetException(wand, &severity);

        printf("save_magickwand_as: Failed to write '%s': %s\n",
                output_path, desc ? desc : "Unknown error");

        if (desc) MagickRelinquishMemory(desc);

        return false;
    }

    printf("[DEBUG] Saved image to '%s' as %s\n", output_path, format);
    return true;
}