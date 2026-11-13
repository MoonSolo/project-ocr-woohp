#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <MagickWand/MagickWand.h>
#include "Pretreatment/preprocess.h"
#include "Treatment/grid_detection/grid_detect.h"
#include "Treatment/grid_size.h"
#include "Treatment/cell_extract.h"
#include "Treatment/word_detection/word_detection.h"

static void create_directory_recursive(const char *path)
{
    char tmp[2048];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    
    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static void create_debug_structure(const char *base_folder)
{
    char path[2048];
    
    create_directory_recursive(base_folder);
    
    snprintf(path, sizeof(path), "%s/0_rotation", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/1_preprocessing", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/2_grid_detection", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path),
             "%s/2_grid_detection/cell_extraction", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/3_word_list", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/4_words", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/5_letters", base_folder);
    create_directory_recursive(path);
    
    snprintf(path, sizeof(path), "%s/4_grid_cells", base_folder);
    create_directory_recursive(path);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <image_path>\n", argv[0]);
        return 1;
    }
    
    const char *image_path = argv[1];
    char *base_name_full = strdup(image_path);
    if (!base_name_full) 
    {
        printf("Failed to duplicate image path\n");
        return 1;
    }

    char *base_name = base_name_full;
    char *dot = strrchr(base_name, '.');
    if (dot) *dot = '\0';
    char *slash = strrchr(base_name, '/');
    if (slash) base_name = slash + 1;
    
    char debug_folder[1024];
    snprintf(debug_folder, sizeof(debug_folder), "DEBUG/%s", base_name);
    
    printf("=== OCR Image Processing Pipeline ===\n");
    printf("Input:    %s\n", image_path);
    printf("Debug folder: %s\n\n", debug_folder);
    
    create_debug_structure(debug_folder);
    
    char rotation_folder[2048], preprocess_folder[2048], grid_folder[2048];
    char word_folder[2048], letter_folder[2048], grid_cells_folder[2048];
    
    snprintf(rotation_folder, sizeof(rotation_folder),
             "%s/0_rotation", debug_folder);
    snprintf(preprocess_folder, sizeof(preprocess_folder),
             "%s/1_preprocessing", debug_folder);
    snprintf(grid_folder, sizeof(grid_folder),
             "%s/2_grid_detection", debug_folder);
    snprintf(word_folder, sizeof(word_folder),
             "%s/3_word_list", debug_folder);
    snprintf(letter_folder, sizeof(letter_folder),
             "%s/5_letters", debug_folder);
    snprintf(grid_cells_folder, sizeof(grid_cells_folder),
             "%s/4_grid_cells", debug_folder);
    
    set_rotation_debug_folder(rotation_folder);
    set_preprocess_debug_folder(preprocess_folder);
    set_grid_detection_debug_folder(grid_folder);
    set_grid_size_debug_folder(grid_folder);
    set_cell_extraction_debug_folder(grid_cells_folder);
    set_word_extract_debug_folder(word_folder);
    
    MagickWandGenesis();
    
    printf("[1/7] Loading image...\n");
    MagickWand *input = NewMagickWand();
    if (MagickReadImage(input, image_path) == MagickFalse)
    {
        fprintf(stderr, "Failed to read image: %s\n", image_path);
        DestroyMagickWand(input);
        MagickWandTerminus();
        return 1;
    }
    
    printf("[2/7] Automatic rotation correction...\n");
    MagickWand *rotated = auto_rotate_image(input);
    DestroyMagickWand(input);
    
    if (! rotated)
    {
        fprintf(stderr, "Rotation correction failed\n");
        MagickWandTerminus();
        return 1;
    }
    
    printf("[3/7] Preprocessing image...\n");
    MagickWand *preprocessed = preprocess_image(rotated);
    if (!preprocessed)
    {
        fprintf(stderr, "Preprocessing failed\n");
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("[4/7] Detecting grid...\n");
    GridDetectionResult grid_result = detect_grid(rotated);
    
    if (! grid_result.detected)
    {
        fprintf(stderr, "Grid detection failed\n");
        DestroyMagickWand(preprocessed);
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("      Grid at (%d, %d), size %dx%d\n",
           grid_result.x, grid_result.y,
           grid_result.width, grid_result.height);
    
    MagickWand *grid_for_cells = CloneMagickWand(rotated);
    MagickCropImage(grid_for_cells, grid_result.width, grid_result.height,
                   grid_result.x, grid_result.y);
    MagickResetImagePage(grid_for_cells, "");
    
    MagickWand *cropped_grid = CloneMagickWand(rotated);
    MagickCropImage(cropped_grid, grid_result.width, grid_result.height,
                   grid_result.x, grid_result.y);
    MagickResetImagePage(cropped_grid, "");
    
    printf("[5/7] Detecting grid size and extracting cells...\n");
    GridSizeResult size_result = detect_grid_size(cropped_grid);
    
    if (!size_result. success)
    {
        fprintf(stderr, "Grid size detection failed\n");
        DestroyMagickWand(grid_for_cells);
        DestroyMagickWand(cropped_grid);
        DestroyMagickWand(preprocessed);
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("      Detected %zux%zu grid\n",
           size_result.rows, size_result.cols);
    
    CellExtractionResult cells = extract_cells(grid_for_cells,
                                               (int)size_result.rows,
                                               (int)size_result.cols);
    
    if (!cells.success)
    {
        fprintf(stderr, "Cell extraction failed\n");
        DestroyMagickWand(grid_for_cells);
        DestroyMagickWand(cropped_grid);
        DestroyMagickWand(preprocessed);
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("      Extracted %zu cells (64x64 each)\n", cells.total_cells);
    
    printf("[6/7] Extracting word list...\n");
    WordListRegionResult word_list_result =
        extract_word_list_with_grid(rotated,
                                    grid_result.x, grid_result.y,
                                    grid_result.width, grid_result.height);
    
    if (!word_list_result.success)
    {
        fprintf(stderr, "Word list extraction failed\n");
        free_cells(&cells);
        DestroyMagickWand(grid_for_cells);
        DestroyMagickWand(cropped_grid);
        DestroyMagickWand(preprocessed);
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("      Word list at (%d, %d), size %dx%d\n",
           word_list_result. x, word_list_result. y,
           word_list_result.width, word_list_result.height);
    
    char word_list_path[2048];
    snprintf(word_list_path, sizeof(word_list_path),
             "%s/3_word_list/word_list.png", debug_folder);
    MagickWriteImage(word_list_result.word_list_region, word_list_path);
    
    char words_folder[2048];
    snprintf(words_folder, sizeof(words_folder),
             "%s/4_words", debug_folder);
    
    WordExtractionResult words_result =
        extract_individual_words(word_list_result.word_list_region,
                                words_folder);
    
    if (!words_result.success)
    {
        fprintf(stderr, "Word extraction failed\n");
        free_word_list_result(&word_list_result);
        free_cells(&cells);
        DestroyMagickWand(grid_for_cells);
        DestroyMagickWand(cropped_grid);
        DestroyMagickWand(preprocessed);
        DestroyMagickWand(rotated);
        MagickWandTerminus();
        return 1;
    }
    
    printf("      Extracted %zu words\n", words_result.word_count);
    
    for (size_t w = 0; w < words_result.word_count; w++)
    {
        char word_path[2048];
        snprintf(word_path, sizeof(word_path),
                 "%s/4_words/word_%02zu.png", debug_folder, w);
        MagickWriteImage(words_result.words[w], word_path);
    }
    
    printf("[7/7] Segmenting letters...\n");
    int total_letters = 0;
    
    for (size_t w = 0; w < words_result.word_count; w++)
    {
        LetterExtractionResult letter_res =
            split_word_into_letters(words_result.words[w],
                                   letter_folder);
        
        if (letter_res.success)
        {
            for (size_t l = 0; l < letter_res.letter_count; l++)
            {
                char letter_filename[2048];
                snprintf(letter_filename, sizeof(letter_filename),
                         "%s/WORD_%02zu_%02zu.png",
                         letter_folder, w, l);
                
                MagickWriteImage(letter_res.letters[l], letter_filename);
            }
            
            total_letters += letter_res.letter_count;
            free_letter_extraction_result(&letter_res);
        }
    }
    
    printf("      Extracted %d letters\n", total_letters);
    
    free_word_extraction_result(&words_result);
    free_word_list_result(&word_list_result);
    free_cells(&cells);
    DestroyMagickWand(grid_for_cells);
    DestroyMagickWand(cropped_grid);
    DestroyMagickWand(preprocessed);
    DestroyMagickWand(rotated);
    
    free(base_name_full);
    MagickWandTerminus();
    
    printf("\n=== Pipeline Complete ===\n");
    printf("\nDebug output structure:\n");
    printf("%s/\n", debug_folder);
    printf("├── 0_rotation/\n");
    printf("│   └── rotation_corrected.png\n");
    printf("├── 1_preprocessing/\n");
    printf("│   ├── 01_grayscale.png\n");
    printf("│   ├── 02_normalized.png\n");
    printf("│   ├── 03_thresholded.png\n");
    printf("│   └── 04_noise_reduced.png\n");
    printf("├── 2_grid_detection/\n");
    printf("│   ├── grid_cropped.png\n");
    printf("│   ├── histogram_horizontal.png\n");
    printf("│   ├── histogram_vertical.png\n");
    printf("│   ├── character_detection_strategy2.png\n");
    printf("│   └── preprocessed.png\n");
    printf("├── 4_grid_cells/\n");
    printf("│   └── GRID_r0_c0.png ...  GRID_r%zu_c%zu.png (64x64 each)\n",
           size_result.rows - 1, size_result.cols - 1);
    printf("├── 3_word_list/\n");
    printf("│   └── word_list.png\n");
    printf("├── 4_words/\n");
    printf("│   └── word_00.png...  word_%02zu.png\n",
           words_result. word_count - 1);
    printf("└── 5_letters/\n");
    printf("    └── WORD_XX_YY.png (%d letters total)\n", total_letters);
    
    
    return 0;
}
