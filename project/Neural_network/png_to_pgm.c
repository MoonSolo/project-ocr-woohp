#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "png_to_pgm.h"


// Convert a single PNG file to PGM using ImageMagick
int convert_png_to_pgm(const char *png_path, const char *pgm_path) {
    char command[1024];
    // Build the command string: "convert input.png output.pgm"
    snprintf(command, sizeof(command),
    "magick \"%s\" "
    "-colorspace Gray "
    "-resize 64x64! "
    "-filter Point "
    "-threshold 60%% "
    "-depth 8 "
    "PGM:\"%s\"",
    png_path, pgm_path);
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error: Failed to convert %s to %s\n", png_path, pgm_path);
        return 0; // failure
    }
    return 1; // success
}

// Convert all PNGs in a folder to PGM
int convert_folder_png_to_pgm(const char *folder_path) {
    char command[1024];
    // Using ImageMagick mogrify to convert all PNGs to PGM in-place
    snprintf(command, sizeof(command), "mogrify -format pgm \"%s\"/*.png", folder_path);
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error: Failed to convert PNGs in folder %s\n", folder_path);
        return 0;
    }
    return 1;
}
