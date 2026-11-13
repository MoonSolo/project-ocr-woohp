#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
} RGBTRIPLE;

void decenter(int height, int width, RGBTRIPLE* image, int offsetX, int offsetY) {
    RGBTRIPLE* temp = calloc(height * width, sizeof(RGBTRIPLE));
    if (!temp) return;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int src_i = i + offsetY;
            int src_j = j + offsetX;
            if (src_i >= 0 && src_i < height && src_j >= 0 && src_j < width)
                temp[i * width + j] = image[src_i * width + src_j];
            else
                temp[i * width + j] = (RGBTRIPLE){255, 255, 255}; // ← blanc
        }
    }

    memcpy(image, temp, height * width * sizeof(RGBTRIPLE));
    free(temp);
}


// Simple box blur (slight performance tweak + clarity)
void blur(int height, int width, RGBTRIPLE* image) {
    RGBTRIPLE* temp = malloc(height * width * sizeof(RGBTRIPLE));
    if (!temp) return;
    memcpy(temp, image, height * width * sizeof(RGBTRIPLE));

    int radius = 1 + rand() % 3; // random blur radius (1–3)
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int red = 0, green = 0, blue = 0, count = 0;
            for (int di = -radius; di <= radius; ++di) {
                for (int dj = -radius; dj <= radius; ++dj) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        int idx = ni * width + nj;
                        red += temp[idx].rgbtRed;
                        green += temp[idx].rgbtGreen;
                        blue += temp[idx].rgbtBlue;
                        count++;
                    }
                }
            }
            int idx = i * width + j;
            image[idx].rgbtRed   = red / count;
            image[idx].rgbtGreen = green / count;
            image[idx].rgbtBlue  = blue / count;
        }
    }

    free(temp);
}

int load_png(const char* filename, RGBTRIPLE** out_img, int* width, int* height) {
    int channels;
    unsigned char* data = stbi_load(filename, width, height, &channels, 0);
    if (!data) {
        fprintf(stderr, "❌ Error loading %s\n", filename);
        return 0;
    }

    int sz = (*width) * (*height);
    *out_img = malloc(sz * sizeof(RGBTRIPLE));
    if (!*out_img) {
        stbi_image_free(data);
        return 0;
    }

    for (int i = 0; i < sz; ++i) {
        unsigned char r = 0, g = 0, b = 0;
        if (channels == 1) {
            // grayscale image
            r = g = b = data[i];
        } else if (channels == 2) {
            // grayscale + alpha (ignore alpha)
            r = g = b = data[i * 2];
        } else {
            // normal RGB or RGBA
            r = data[i * channels + 0];
            g = data[i * channels + 1];
            b = data[i * channels + 2];
        }
        (*out_img)[i].rgbtRed   = r;
        (*out_img)[i].rgbtGreen = g;
        (*out_img)[i].rgbtBlue  = b;
    }

    stbi_image_free(data);
    return 1;
}


int save_png(const char* filename, RGBTRIPLE* image, int width, int height) {
    unsigned char* bytes = malloc(width * height * 3);
    if (!bytes) return 0;
    for (int i = 0; i < width * height; ++i) {
        bytes[i*3+0] = image[i].rgbtRed;
        bytes[i*3+1] = image[i].rgbtGreen;
        bytes[i*3+2] = image[i].rgbtBlue;
    }

    int res = stbi_write_png(filename, width, height, 3, bytes, width * 3);
    free(bytes);
    return res;
}

void process_image(const char* input_path, const char* output_dir, int nbToGenerate) {
    RGBTRIPLE* image = NULL;
    int width, height;
    if (!load_png(input_path, &image, &width, &height))
        return;

    mkdir(output_dir, 0777);

    for (int i = 0; i < nbToGenerate; ++i) {
        RGBTRIPLE* buffer = malloc(width * height * sizeof(RGBTRIPLE));
        if (!buffer) { free(image); return; }
        memcpy(buffer, image, width * height * sizeof(RGBTRIPLE));

        // Random recentring offset, limited to 25% of the image size
        int offsetX = (rand() % (width / 2)) - (width / 4);
        int offsetY = (rand() % (height / 2)) - (height / 4);

        decenter(height, width, buffer, offsetX, offsetY);
        blur(height, width, buffer);

        char output_filename[512];
        snprintf(output_filename, sizeof(output_filename), "%s/%s_%02d.png", output_dir, strrchr(input_path, '/') ? strrchr(input_path, '/') + 1 : input_path, i);
        save_png(output_filename, buffer, width, height);
        printf("✅ Generated: %s (offset %d,%d)\n", output_filename, offsetX, offsetY);
        free(buffer);
    }

    free(image);
}

void process_folder(const char* root, int nbToGenerate) {
    DIR* dir = opendir(root);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            // It's a subdirectory (like A, B, etc.)
            char outdir[1024];
            snprintf(outdir, sizeof(outdir), "datas/%s", entry->d_name);
            mkdir(outdir, 0777);

            // Recurse into the subfolder
            process_folder(path, nbToGenerate);
        } else {
            // It's an image file
            int len = strlen(entry->d_name);
            if (len > 4 && strcmp(entry->d_name + len - 4, ".png") == 0) {
                // Compute parent folder name for output path
                const char* last_slash = strrchr(root, '/');
                const char* folder_name = last_slash ? last_slash + 1 : root;

                char outdir[1024];
                snprintf(outdir, sizeof(outdir), "datas/%s", folder_name);
                mkdir(outdir, 0777);

                process_image(path, outdir, nbToGenerate);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s dataset_folder\n", argv[0]);
        return 1;
    }
    srand(time(NULL));
    int nbToGenerate = 50;
    process_folder(argv[1], nbToGenerate);
    printf("\n🎉 All images generated in datas/!\n");
    return 0;
}

