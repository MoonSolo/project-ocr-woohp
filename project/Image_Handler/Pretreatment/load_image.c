#include "preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void imagemagick_init(void) 
{
    printf("[DEBUG] : Initializing ImageMagick\n");
    MagickWandGenesis();
}

void imagemagick_terminate(void) 
{
    printf("[DEBUG] : Terminating ImageMagick "
           "- Make sure not to call it without re-initializing it!\n");
    MagickWandTerminus();
}

LoadImageResult load_image(const char *path)
{
    printf("[DEBUG] : Called load_image().\n");
    LoadImageResult result;
    result.wand = NULL;
    result.error[0] = '\0'; 
    result.severity = UndefinedException;

    if (!path) {
        snprintf(result.error, sizeof(result.error),
                 "load_image: Image path is NULL.\n");
        return result;
    }

    if (access(path, R_OK) != 0) {
        snprintf(result.error, sizeof(result.error),
                 "load_image: Cannot access file '%s': %s.\n",
                 path, strerror(errno));
        return result;
    }

    MagickWand *wand = NewMagickWand();
    if (!wand) {
        snprintf(result.error, sizeof(result.error),
                 "load_image: NewMagickWand() returned NULL.\n");
        return result;
    }

    if (MagickReadImage(wand, path) == MagickFalse) {
        ExceptionType severity;
        char *desc = MagickGetException(wand, &severity);

        if (desc && *desc) {
            snprintf(result.error, sizeof(result.error),
                     "load_image: MagickReadImage('%s') failed: %s.\n",
                     path, desc);
        } else {
            snprintf(result.error, sizeof(result.error),
                     "load_image: MagickReadImage('%s') failed with unknown"
					 "error. "
                     "Check permissions or ImageMagick policy.xml\n", path);
        }

        result.severity = severity;
        if (desc) MagickRelinquishMemory(desc);
        
        DestroyMagickWand(wand);
        return result;
    }

    result.wand = wand;
    printf("[DEBUG] : load_image() operated successfully without any error.\n");
    return result;
}
