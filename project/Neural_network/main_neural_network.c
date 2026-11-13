#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h> 
#include <dirent.h>
#include "neural_network.h"
#include "image_converter.h"
#include "png_to_pgm.h"

// Dataset Configuration 
#define NUM_LETTERS 52 
#define SAMPLES_PER_LETTER 2000
#define TRAIN_SAMPLES_PER_LETTER (SAMPLES_PER_LETTER / 2) 
#define TEST_SAMPLES_PER_LETTER (SAMPLES_PER_LETTER / 2)  

#define TOTAL_TRAIN_SAMPLES (NUM_LETTERS * TRAIN_SAMPLES_PER_LETTER)
#define TOTAL_TEST_SAMPLES (NUM_LETTERS * TEST_SAMPLES_PER_LETTER)

const char *DATASET_BASE_PATH = "../Neural_network/Training/dataset/";
const char *MODEL_FILE = "../Neural_network/letter_model.net";

// Grid Processing Configuration
#define MAX_GRID_X 100
#define MAX_GRID_Y 100

typedef struct {
    char path[256];
    int label;
} DataItem;

// Helper Functions 

void get_letter_dir_from_index(int index, char *buffer) {
    if (index < 26) {
        buffer[0] = 'A' + index;
        buffer[1] = '\0';
    } else {
        buffer[0] = 'a' + (index - 26);
        buffer[1] = '_';
        buffer[2] = '\0';
    }
}

char get_char_from_index(int index) {
    return 'A' + (index % 26);
}

void prepare_data_items(DataItem *items, int start_file_index, int num_files) {
    int item_index = 0;
    char letter_dir[4];
    for (int i = 0; i < NUM_LETTERS; i++) {
        get_letter_dir_from_index(i, letter_dir);
        for (int j = 0; j < num_files; j++) {
            sprintf(items[item_index].path, "%s%s/%d.pgm", 
                    DATASET_BASE_PATH, letter_dir, start_file_index + j);
            items[item_index].label = i;
            item_index++;
        }
    }
    printf("Prepared %d data items (from file %d to %d).\n", 
            item_index, start_file_index, start_file_index + num_files - 1);
}

void shuffle_data(DataItem *array, size_t n) {
    if (n > 1) {
        for (size_t i = n - 1; i > 0; i--) {
            size_t j = (size_t)(rand() / (RAND_MAX / (double)(i + 1)));
            DataItem temp = array[j];
            array[j] = array[i];
            array[i] = temp;
        }
    }
    printf("Shuffled %zu data items.\n", n);
}

void load_all_training_data(DataItem *items, int num_samples, float **all_inputs, float **all_targets) {
    *all_inputs = malloc(num_samples * INPUT_SIZE * sizeof(float));
    *all_targets = malloc(num_samples * OUTPUT_SIZE * sizeof(float));

    if (!*all_inputs || !*all_targets) {
        perror("Failed to allocate memory for training data");
        exit(EXIT_FAILURE);
    }

    memset(*all_targets, 0, num_samples * OUTPUT_SIZE * sizeof(float));

    printf("Loading %d training images into memory... (This may take a while)\n", num_samples);
    for (int i = 0; i < num_samples; i++) {
        float *image_vector = load_pgm_as_vector(items[i].path);
        if (!image_vector) {
            fprintf(stderr, "Failed to load image: %s. Exiting.\n", items[i].path);
            free(*all_inputs);
            free(*all_targets);
            exit(EXIT_FAILURE);
        }

        memcpy(&((*all_inputs)[i * INPUT_SIZE]), image_vector, INPUT_SIZE * sizeof(float));
        free(image_vector); 

        int label = items[i].label;
        (*all_targets)[i * OUTPUT_SIZE + label] = 1.0f;

        if ((i + 1) % 5000 == 0) {
            printf("... loaded %d / %d images\n", i + 1, num_samples);
        }
    }
    printf("All training data loaded.\n");
}

void run_tests(NeuralNetwork *net, DataItem *test_items) {
    printf("Running tests on %d images...\n", TOTAL_TEST_SAMPLES);
    int correct_count = 0;
    
    for (int i = 0; i < TOTAL_TEST_SAMPLES; i++) {
        float *image_vector = load_pgm_as_vector(test_items[i].path);
        if (!image_vector) {
            fprintf(stderr, "Warning: Failed to load test image %s. Skipping.\n", test_items[i].path);
            continue;
        }

        int correct_label = test_items[i].label;
        int predicted_label = predict(net, image_vector);

        if (predicted_label == correct_label) {
            correct_count++;
        }

        free(image_vector);

        if ((i + 1) % 5000 == 0) {
            printf("... tested %d / %d images\n", i + 1, TOTAL_TEST_SAMPLES);
        }
    }

    float accuracy = (float)correct_count / TOTAL_TEST_SAMPLES * 100.0f;
    printf("----------------------------------------\n");
    printf("Test Complete.\n");
    printf("Correct: %d / %d\n", correct_count, TOTAL_TEST_SAMPLES);
    printf("Accuracy: %.2f%%\n", accuracy);
    printf("----------------------------------------\n");
}

// NEW FUNCTION: Process Folder to Grid
void process_grid_folder(NeuralNetwork *net, const char *folder_path, const char *output_file) {
    printf("----------------------------------------\n");
    printf("Processing Grid Folder: %s\n", folder_path);

    // Initialize Grid with spaces
    static char grid[MAX_GRID_X][MAX_GRID_Y];
    for(int y=0; y<MAX_GRID_Y; y++) {
        for(int x=0; x<MAX_GRID_X; x++) {
            grid[x][y] = ' ';
        }
    }

    // Open Directory
    DIR *d;
    struct dirent *dir;
    d = opendir(folder_path);
    if (!d) {
        perror("Error opening grid folder");
        return;
    }

	while ((dir = readdir(d)) != NULL) {
		char *filename = dir->d_name;
		size_t len = strlen(filename);

		if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
			continue;

		if (len <= 4 || strcmp(filename + len - 4, ".png") != 0)
			continue;

		char name_only[256];
		strncpy(name_only, filename, len - 4);
		name_only[len - 4] = '\0';

		char png_path[512];
		char pgm_path[512];

		snprintf(png_path, sizeof(png_path), "%s/%s", folder_path, filename);
		snprintf(pgm_path, sizeof(pgm_path), "%s/%s.pgm", folder_path, name_only);

		if (convert_png_to_pgm(png_path, pgm_path)) {
			// remove(png_path); erase PNG after successful conversion
		} else {
			fprintf(stderr, "Failed to convert PNG: %s\n", png_path);
		}
	}
	closedir(d);

	d = opendir(folder_path);
	if (!d) {
		perror("Error reopening grid folder");
		return;
	}

    int files_processed = 0;
    int max_x_found = 0;
    int max_y_found = 0;

    // Iterate files
    while ((dir = readdir(d)) != NULL) {
        int x, y;
        char *filename = dir->d_name;
        size_t len = strlen(filename);

        // Skip . and ..
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        // Determine extension
        int is_pgm = (len > 4 && strcmp(filename + len - 4, ".pgm") == 0);
        int is_png = (len > 4 && strcmp(filename + len - 4, ".png") == 0);

        if (!is_pgm && !is_png)
            continue;

        // Extract coordinates from filename (strip extension)
        char name_only[256];
        strncpy(name_only, filename, len - 4);
        name_only[len - 4] = '\0';

        if ((sscanf(name_only, "GRID_%d_%d", &y, &x) != 2) &&
			(sscanf(name_only, "WORD_%d_%d", &y, &x) != 2))
		{
            printf("Warning: Invalid filename format: %s\n", filename);
            continue;
        }

        if (x < 0 || x >= MAX_GRID_X || y < 0 || y >= MAX_GRID_Y) {
            printf("Warning: Coordinates %d,%d out of bounds (Max %d,%d)\n", x, y, MAX_GRID_X, MAX_GRID_Y);
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, filename);

        // Convert PNG to PGM if needed
        if (is_png) {
            char pgm_path[512];
            snprintf(pgm_path, sizeof(pgm_path), "%s/%s.pgm", folder_path, name_only);

            if (!convert_png_to_pgm(full_path, pgm_path)) {
                fprintf(stderr, "Failed to convert PNG to PGM: %s\n", full_path);
                continue;
            }

            strcpy(full_path, pgm_path); // Now full_path points to the PGM
        }

        // Load and predict
        float *image_vector = load_pgm_as_vector(full_path);
        if (image_vector) {
            int predicted_index = predict(net, image_vector);
            char predicted_char = get_char_from_index(predicted_index);

            grid[x][y] = predicted_char;
            free(image_vector);
            files_processed++;

            if (x > max_x_found) max_x_found = x;
            if (y > max_y_found) max_y_found = y;
        }
    }
    closedir(d);

    printf("Processed %d files. Reconstructing text...\n", files_processed);

    // Write to Output File
    FILE *fp = fopen(output_file, "w");
    if (!fp) {
        perror("Error opening output file");
        return;
    }

    for (int y = 0; y <= max_y_found; y++) {
        for (int x = 0; x <= max_x_found; x++) {
            fputc(grid[x][y], fp);
        }
        fputc('\n', fp);
    }

    fclose(fp);
    printf("Text reconstruction saved to: %s\n", output_file);
    printf("----------------------------------------\n");
}


// Main Function
int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Alocates memory early (only used in training mode)
    DataItem *training_items = malloc(TOTAL_TRAIN_SAMPLES * sizeof(DataItem));
    DataItem *testing_items = malloc(TOTAL_TEST_SAMPLES * sizeof(DataItem));
    if (!training_items || !testing_items) {
        perror("Failed to allocate DataItem arrays");
        return 1;
    }

    // Prepare items structures (needed for training/testing), but not loaded
    prepare_data_items(training_items, 0, TRAIN_SAMPLES_PER_LETTER);
    prepare_data_items(testing_items, TRAIN_SAMPLES_PER_LETTER, TEST_SAMPLES_PER_LETTER);

    NeuralNetwork *net = init_network();

    // DIRECT CLI MODE CHECK 
    if (argc > 1) {
        printf("--- Direct Grid Processing Mode ---\n");
        
        // Load the model immediately
        load_network(net, MODEL_FILE);

        // Get folder path
        const char *folder_path = argv[1];
        
        // Get output path (default if not provided)
        const char *output_path = (argc > 2) ? argv[2] : "reconstructed_text.txt";

        // Process
        process_grid_folder(net, folder_path, output_path);

        // Cleanup and Exit
        free_network(net);
        free(training_items);
        free(testing_items);
        return 0;
    }

    float *all_inputs = NULL;
    float *all_targets = NULL;
    int data_loaded = 0; 
    int total_epochs_trained = 0;

    // Default paths for the grid folder
    char grid_folder_path[256] = "grid_images"; 
    char grid_output_path[256] = "reconstructed_text.txt";

    char choice = ' ';
    while (choice != 'q') {
        printf("\n--- OCR Neural Network Main Menu ---\n");
        printf("Total epochs trained: %d\n", total_epochs_trained);
        printf("[d] Load training data (required before training)\n");
        printf("[t] Train network for %d epochs\n", EPOCHS_PER_CHUNK);
        printf("[f] Free training data from memory\n");
        printf("[l] Load a saved model from file (%s)\n", MODEL_FILE);
        printf("[r] Run tests on the test dataset\n");
        printf("[g] Process a folder of grid images (Production Mode)\n");
        printf("[s] Save the current model to file (%s)\n", MODEL_FILE);
        printf("[q] Quit\n");
        printf("Enter your choice: ");
        
        (void)scanf(" %c", &choice); 
        while(getchar() != '\n'); 

        switch (tolower(choice)) {
            case 'd':
                if (data_loaded) {
                    printf("Freeing previously loaded data...\n");
                    free(all_inputs);
                    free(all_targets);
                    all_inputs = NULL;
                    all_targets = NULL;
                    data_loaded = 0;
                }
                printf("Preparing to load training data...\n");
                shuffle_data(training_items, TOTAL_TRAIN_SAMPLES);
                load_all_training_data(training_items, TOTAL_TRAIN_SAMPLES, &all_inputs, &all_targets);
                data_loaded = 1;
                break;

            case 't': {
                if (!data_loaded) {
                    printf("Error: Training data is not loaded. Use [d] first.\n");
                    break;
                }
                printf("Preparing to train for %d epochs...\n", EPOCHS_PER_CHUNK);
                train(net, all_inputs, all_targets, TOTAL_TRAIN_SAMPLES);
                total_epochs_trained += EPOCHS_PER_CHUNK;
                save_network(net, MODEL_FILE);
                break;
            }

            case 'f':
                if (data_loaded) {
                    printf("Freeing loaded training data...\n");
                    free(all_inputs);
                    free(all_targets);
                    all_inputs = NULL;
                    all_targets = NULL;
                    data_loaded = 0;
                }
                break;

            case 'l':
                load_network(net, MODEL_FILE);
                total_epochs_trained = 0;
                break;

            case 'r':
                run_tests(net, testing_items);
                break;

            case 'g':
                printf("Enter path to grid images folder (default: %s): ", grid_folder_path);
                char input_buffer[256];
                fgets(input_buffer, sizeof(input_buffer), stdin);
                input_buffer[strcspn(input_buffer, "\n")] = 0;
                if (strlen(input_buffer) > 0) {
                    strcpy(grid_folder_path, input_buffer);
                }
                process_grid_folder(net, grid_folder_path, grid_output_path);
                break;

            case 's':
                save_network(net, MODEL_FILE);
                break;

            case 'q':
                printf("Quitting.\n");
                break;

            default:
                printf("Invalid choice.\n");
        }
    }

    free_network(net);
    free(training_items);
    free(testing_items);
    if (data_loaded) {
        free(all_inputs);
        free(all_targets);
    }

    return 0;
}
