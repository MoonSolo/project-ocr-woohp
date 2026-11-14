#include "cell_extract.h"
#include "cell_reprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/stat.h>

static char debug_folder[1024] = "";

void set_cell_extraction_debug_folder(const char *folder)
{
    if (folder)
        snprintf(debug_folder, sizeof(debug_folder), "%s", folder);
    else
        debug_folder[0] = '\0';
}

typedef struct
{
    int x, y, width, height;
    int center_x, center_y;
} BoundingBox;

typedef struct
{
    BoundingBox *boxes;
    int count;
    int capacity;
} BoxList;

typedef struct
{
    int x, y;
} Point;

typedef struct
{
    Point *data;
    int size;
    int capacity;
} Stack;

static Stack* create_stack(void)
{
    Stack *s = (Stack*)malloc(sizeof(Stack));
    if (! s) return NULL;
    s->capacity = 10000;
    s->size = 0;
    s->data = (Point*)malloc(s->capacity * sizeof(Point));
    if (!s->data)
    {
        free(s);
        return NULL;
    }
    return s;
}

static void push(Stack *s, int x, int y)
{
    if (s->size >= s->capacity)
    {
        s->capacity *= 2;
        Point *new_data = (Point*)realloc(s->data, s->capacity * sizeof(Point));
        if (!new_data) return;
        s->data = new_data;
    }
    s->data[s->size].x = x;
    s->data[s->size].y = y;
    s->size++;
}

static int pop(Stack *s, int *x, int *y)
{
    if (s->size == 0) return 0;
    s->size--;
    *x = s->data[s->size]. x;
    *y = s->data[s->size].y;
    return 1;
}

static void free_stack(Stack *s)
{
    if (s)
    {
        free(s->data);
        free(s);
    }
}

static BoxList* create_box_list(void)
{
    BoxList *list = (BoxList*)malloc(sizeof(BoxList));
    if (!list) return NULL;
    
    list->capacity = 500;
    list->count = 0;
    list->boxes = (BoundingBox*)malloc(list->capacity * sizeof(BoundingBox));
    
    if (!list->boxes)
    {
        free(list);
        return NULL;
    }
    
    return list;
}

static void add_box(BoxList *list, BoundingBox box)
{
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        BoundingBox *new_boxes = (BoundingBox*)realloc(
            list->boxes, list->capacity * sizeof(BoundingBox));
        if (!new_boxes) return;
        list->boxes = new_boxes;
    }
    
    list->boxes[list->count++] = box;
}

static void free_box_list(BoxList *list)
{
    if (list)
    {
        free(list->boxes);
        free(list);
    }
}

static void flood_fill_iterative(unsigned char *pixels, int *labels,
                                 int width, int height, int start_x,
                                 int start_y, int label,
                                 int *min_x, int *max_x,
                                 int *min_y, int *max_y)
{
    Stack *stack = create_stack();
    if (!stack) return;
    
    push(stack, start_x, start_y);
    
    while (stack->size > 0)
    {
        int x, y;
        if (! pop(stack, &x, &y)) break;
        
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (labels[y * width + x] != 0) continue;
        if (pixels[y * width + x] > 128) continue;
        
        labels[y * width + x] = label;
        
        if (x < *min_x) *min_x = x;
        if (x > *max_x) *max_x = x;
        if (y < *min_y) *min_y = y;
        if (y > *max_y) *max_y = y;
        
        push(stack, x + 1, y);
        push(stack, x - 1, y);
        push(stack, x, y + 1);
        push(stack, x, y - 1);
    }
    
    free_stack(stack);
}

static BoxList* find_characters(unsigned char *pixels, int width, int height)
{
    BoxList *boxes = create_box_list();
    if (!boxes) return NULL;
    
    int *labels = (int*)calloc(width * height, sizeof(int));
    if (!labels)
    {
        free_box_list(boxes);
        return NULL;
    }
    
    int current_label = 1;
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (pixels[y * width + x] < 128 && labels[y * width + x] == 0)
            {
                int min_x = width, max_x = 0;
                int min_y = height, max_y = 0;
                
                flood_fill_iterative(pixels, labels, width, height, x, y,
                                    current_label, &min_x, &max_x,
                                    &min_y, &max_y);
                
                int w = max_x - min_x + 1;
                int h = max_y - min_y + 1;
                
                if (w >= 3 && w <= 80 && h >= 3 && h <= 80)
                {
                    float aspect = (float)w / (float)h;
                    
                    if (aspect > 0.1f && aspect < 10.0f)
                    {
                        BoundingBox box;
                        box.x = min_x;
                        box. y = min_y;
                        box.width = w;
                        box.height = h;
                        box.center_x = (min_x + max_x) / 2;
                        box.center_y = (min_y + max_y) / 2;
                        
                        add_box(boxes, box);
                    }
                }
                
                current_label++;
            }
        }
    }
    
    free(labels);
    return boxes;
}

static BoundingBox find_character_in_region(MagickWand *image, int region_x, int region_y,
                                            int region_w, int region_h)
{
    BoundingBox result = {-1, -1, 0, 0, -1, -1};
    
    MagickWand *region = CloneMagickWand(image);
    if (!region) return result;
    
    if (MagickCropImage(region, region_w, region_h, region_x, region_y) == MagickFalse)
    {
        DestroyMagickWand(region);
        return result;
    }
    
    MagickResetImagePage(region, "");
    
    MagickTransformImageColorspace(region, GRAYColorspace);
    MagickAdaptiveThresholdImage(region, 15, 15, 0);
    
    unsigned char *pixels = (unsigned char*)malloc(region_w * region_h);
    if (!pixels)
    {
        DestroyMagickWand(region);
        return result;
    }
    
    MagickExportImagePixels(region, 0, 0, region_w, region_h, "I", CharPixel, pixels);
    DestroyMagickWand(region);
    
    int *labels = (int*)calloc(region_w * region_h, sizeof(int));
    if (!labels)
    {
        free(pixels);
        return result;
    }
    
    typedef struct {
        int min_x, max_x, min_y, max_y;
        int pixel_count;
    } Component;
    
    Component components[50];
    int component_count = 0;
    int current_label = 1;
    
    for (int y = 0; y < region_h && component_count < 50; y++)
    {
        for (int x = 0; x < region_w && component_count < 50; x++)
        {
            if (pixels[y * region_w + x] < 128 && labels[y * region_w + x] == 0)
            {
                int min_x = region_w, max_x = 0;
                int min_y = region_h, max_y = 0;
                
                Stack *stack = create_stack();
                if (!stack) continue;
                
                push(stack, x, y);
                int pixel_count = 0;
                
                while (stack->size > 0)
                {
                    int cx, cy;
                    if (! pop(stack, &cx, &cy)) break;
                    
                    if (cx < 0 || cx >= region_w || cy < 0 || cy >= region_h) continue;
                    if (labels[cy * region_w + cx] != 0) continue;
                    if (pixels[cy * region_w + cx] > 128) continue;
                    
                    labels[cy * region_w + cx] = current_label;
                    pixel_count++;
                    
                    if (cx < min_x) min_x = cx;
                    if (cx > max_x) max_x = cx;
                    if (cy < min_y) min_y = cy;
                    if (cy > max_y) max_y = cy;
                    
                    push(stack, cx + 1, cy);
                    push(stack, cx - 1, cy);
                    push(stack, cx, cy + 1);
                    push(stack, cx, cy - 1);
                }
                
                free_stack(stack);
                
                int w = max_x - min_x + 1;
                int h = max_y - min_y + 1;
                
                if (w >= 3 && w <= region_w - 2 && 
                    h >= 3 && h <= region_h - 2 &&
                    pixel_count >= 10)
                {
                    float aspect = (float)w / (float)h;
                    if (aspect > 0.1f && aspect < 10.0f)
                    {
                        components[component_count].min_x = min_x;
                        components[component_count].max_x = max_x;
                        components[component_count].min_y = min_y;
                        components[component_count]. max_y = max_y;
                        components[component_count].pixel_count = pixel_count;
                        component_count++;
                    }
                }
                
                current_label++;
            }
        }
    }
    
    free(labels);
    free(pixels);
    
    if (component_count > 0)
    {
        int largest_idx = 0;
        int max_pixels = components[0].pixel_count;
        
        for (int i = 1; i < component_count; i++)
        {
            if (components[i].pixel_count > max_pixels)
            {
                max_pixels = components[i].pixel_count;
                largest_idx = i;
            }
        }
        
        int center_x = region_w / 2;
        int center_y = region_h / 2;
        
        for (int i = 0; i < component_count; i++)
        {
            int comp_center_x = (components[i].min_x + components[i].max_x) / 2;
            int comp_center_y = (components[i].min_y + components[i].max_y) / 2;
            
            int dist_current = abs(comp_center_x - center_x) + abs(comp_center_y - center_y);
            int dist_largest = abs((components[largest_idx]. min_x + components[largest_idx]. max_x) / 2 - center_x) +
                              abs((components[largest_idx].min_y + components[largest_idx].max_y) / 2 - center_y);
            
            if (components[i].pixel_count >= max_pixels * 0.7 && dist_current < dist_largest * 0.5)
            {
                largest_idx = i;
                max_pixels = components[i].pixel_count;
            }
        }
        
        result.x = region_x + components[largest_idx].min_x;
        result.y = region_y + components[largest_idx].min_y;
        result.width = components[largest_idx].max_x - components[largest_idx].min_x + 1;
        result. height = components[largest_idx]. max_y - components[largest_idx].min_y + 1;
        result.center_x = region_x + (components[largest_idx].min_x + components[largest_idx].max_x) / 2;
        result.center_y = region_y + (components[largest_idx].min_y + components[largest_idx].max_y) / 2;
    }
    
    return result;
}

static int compare_boxes_by_y(const void *a, const void *b)
{
    const BoundingBox *box_a = (const BoundingBox*)a;
    const BoundingBox *box_b = (const BoundingBox*)b;
    return box_a->center_y - box_b->center_y;
}

static int compare_boxes_by_x(const void *a, const void *b)
{
    const BoundingBox *box_a = (const BoundingBox*)a;
    const BoundingBox *box_b = (const BoundingBox*)b;
    return box_a->center_x - box_b->center_x;
}

static void assign_to_grid(BoxList *boxes, int rows, int cols, 
                          MagickWand **grid_cells, MagickWand *original,
                          int grid_width, int grid_height)
{
    if (boxes->count == 0) return;
    
    printf("[INFO] Assigning %d detected characters to %dx%d grid\n", 
           boxes->count, rows, cols);
    
    qsort(boxes->boxes, boxes->count, sizeof(BoundingBox), compare_boxes_by_y);
    
    int row_tolerance = grid_height / (rows * 2); 
    
    BoundingBox **row_groups = (BoundingBox**)calloc(rows, sizeof(BoundingBox*));
    int *row_counts = (int*)calloc(rows, sizeof(int));
    int *row_capacities = (int*)calloc(rows, sizeof(int));
    
    for (int r = 0; r < rows; r++)
    {
        row_capacities[r] = cols + 10;  
        row_groups[r] = (BoundingBox*)calloc(row_capacities[r], sizeof(BoundingBox));
    }
    
    int current_row = 0;
    int last_y = boxes->boxes[0].center_y;
    
    for (int i = 0; i < boxes->count; i++)
    {
        BoundingBox *box = &boxes->boxes[i];
        
        if (i > 0 && box->center_y - last_y > row_tolerance)
        {
            current_row++;
            if (current_row >= rows)
            {
                printf("[WARN] More rows detected than expected (%d > %d)\n", 
                       current_row + 1, rows);
                current_row = rows - 1;
            }
        }
        
        if (row_counts[current_row] < row_capacities[current_row])
        {
            row_groups[current_row][row_counts[current_row]] = *box;
            row_counts[current_row]++;
        }
        
        last_y = box->center_y;
    }
    
    printf("[INFO] Grouped into %d rows\n", current_row + 1);
    
    for (int r = 0; r < rows; r++)
    {
        if (row_counts[r] == 0) continue;
        
        qsort(row_groups[r], row_counts[r], sizeof(BoundingBox), compare_boxes_by_x);
        
        for (int c = 0; c < row_counts[r] && c < cols; c++)
        {
            BoundingBox *box = &row_groups[r][c];
            

            int pad = 3;
            int x = (box->x > pad) ? (box->x - pad) : 0;
            int y = (box->y > pad) ? (box->y - pad) : 0;
            int w = box->width + 2 * pad;
            int h = box->height + 2 * pad;
            
            MagickWand *cell = CloneMagickWand(original);
            if (cell && MagickCropImage(cell, w, h, x, y) != MagickFalse)
            {
                MagickResetImagePage(cell, "");
                
                int grid_index = r * cols + c;
                
                if (grid_index < rows * cols)
                {
                    grid_cells[grid_index] = cell;
                }
                else
                {
                    DestroyMagickWand(cell);
                }
            }
        }
        
        if (row_counts[r] != cols)
        {
            printf("[WARN] Row %d has %d characters (expected %d)\n", 
                   r, row_counts[r], cols);
        }
    }
    
    for (int r = 0; r < rows; r++)
        free(row_groups[r]);
    free(row_groups);
    free(row_counts);
    free(row_capacities);
}

static CellExtractionResult extract_cells_fallback(MagickWand *grid_image, int rows, int cols)
{
    CellExtractionResult result = {0};
    result.success = 0;
    
    size_t grid_width = MagickGetImageWidth(grid_image);
    size_t grid_height = MagickGetImageHeight(grid_image);
    
    int cell_width = grid_width / cols;
    int cell_height = grid_height / rows;
    
    size_t total_cells = (size_t)(rows * cols);
    MagickWand **cells = (MagickWand**)calloc(total_cells, sizeof(MagickWand*));
    
    if (!cells) return result;
    
    int margin_x = cell_width / 5;
    int margin_y = cell_height / 5;
    
    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            int x = col * cell_width + margin_x;
            int y = row * cell_height + margin_y;
            int w = cell_width - 2 * margin_x;
            int h = cell_height - 2 * margin_y;
            
            if (w > 0 && h > 0)
            {
                MagickWand *cell = CloneMagickWand(grid_image);
                if (cell && MagickCropImage(cell, w, h, x, y) != MagickFalse)
                {
                    MagickResetImagePage(cell, "");
                    size_t index = (size_t)(row * cols + col);
                    cells[index] = cell;
                }
            }
        }
    }
    
    reprocess_all_cells(cells, total_cells);
    
    result.success = 1;
    result.cells = cells;
    result.rows = (size_t)rows;
    result.cols = (size_t)cols;
    result.total_cells = total_cells;
    
    return result;
}

CellExtractionResult extract_cells(MagickWand *grid_image, int rows, int cols)
{
    CellExtractionResult result = {0};
    result.success = 0;
    
    if (! grid_image || rows <= 0 || cols <= 0)
    {
        fprintf(stderr, "[CELL] ERROR: Invalid input parameters\n");
        return result;
    }
    
    size_t grid_width = MagickGetImageWidth(grid_image);
    size_t grid_height = MagickGetImageHeight(grid_image);
    
    printf("[INFO] Extracting %dx%d grid from %zux%zu image using character detection\n",
           rows, cols, grid_width, grid_height);
    
    MagickWand *processed = CloneMagickWand(grid_image);
    MagickTransformImageColorspace(processed, GRAYColorspace);
    MagickAdaptiveThresholdImage(processed, 15, 15, 0);
    
    unsigned char *pixels = (unsigned char*)malloc(grid_width * grid_height);
    if (!pixels)
    {
        fprintf(stderr, "[CELL] ERROR: Failed to allocate pixel buffer\n");
        DestroyMagickWand(processed);
        return result;
    }
    
    MagickExportImagePixels(processed, 0, 0, grid_width, grid_height, "I",
                           CharPixel, pixels);
    DestroyMagickWand(processed);
    
    BoxList *boxes = find_characters(pixels, grid_width, grid_height);
    free(pixels);
    
    if (!boxes || boxes->count < rows * cols * 0.5)
    {
        fprintf(stderr, "[CELL] WARN: Found only %d characters (expected ~%d)\n",
                boxes ?  boxes->count : 0, rows * cols);
        
        if (boxes) free_box_list(boxes);
        
        printf("[INFO] Falling back to simple grid division\n");
        return extract_cells_fallback(grid_image, rows, cols);
    }
    
    printf("[INFO] Found %d characters\n", boxes->count);
    
    size_t total_cells = (size_t)(rows * cols);
    MagickWand **cells = (MagickWand**)calloc(total_cells, sizeof(MagickWand*));
    
    if (!cells)
    {
        fprintf(stderr, "[CELL] ERROR:  Failed to allocate memory for cells\n");
        free_box_list(boxes);
        return result;
    }
    
    assign_to_grid(boxes, rows, cols, cells, grid_image, grid_width, grid_height);
    
    free_box_list(boxes);
    
    printf("[INFO] Filling missing cells with smart fallback extraction...\n");
    
    int filled_count = 0;
    int cell_width = grid_width / cols;
    int cell_height = grid_height / rows;
    
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r * cols + c;
            
            if (cells[idx] == NULL)
            {
                int region_x = c * cell_width;
                int region_y = r * cell_height;
                int region_w = cell_width;
                int region_h = cell_height;
                
                if (region_x + region_w > (int)grid_width) 
                    region_w = grid_width - region_x;
                if (region_y + region_h > (int)grid_height) 
                    region_h = grid_height - region_y;
                
                BoundingBox char_box = find_character_in_region(grid_image, 
                                                                region_x, region_y,
                                                                region_w, region_h);
                
                if (char_box.x >= 0 && char_box. width > 0)
                {
                    int pad = 3;
                    int x = (char_box.x > pad) ? (char_box.x - pad) : 0;
                    int y = (char_box.y > pad) ? (char_box.y - pad) : 0;
                    int w = char_box. width + 2 * pad;
                    int h = char_box.height + 2 * pad;
                    
                    MagickWand *cell = CloneMagickWand(grid_image);
                    if (cell && MagickCropImage(cell, w, h, x, y) != MagickFalse)
                    {
                        MagickResetImagePage(cell, "");
                        cells[idx] = cell;
                        filled_count++;
                    }
                }
                else
                {
                    int margin_x = cell_width / 5;
                    int margin_y = cell_height / 5;
                    
                    int x = region_x + margin_x;
                    int y = region_y + margin_y;
                    int w = region_w - 2 * margin_x;
                    int h = region_h - 2 * margin_y;
                    
                    if (w > 0 && h > 0)
                    {
                        MagickWand *cell = CloneMagickWand(grid_image);
                        if (cell && MagickCropImage(cell, w, h, x, y) != MagickFalse)
                        {
                            MagickResetImagePage(cell, "");
                            cells[idx] = cell;
                            filled_count++;
                        }
                    }
                }
            }
        }
    }
    
    if (filled_count > 0)
    {
        printf("[INFO] ✓ Filled %d missing cells with smart fallback\n", filled_count);
    }
    
    printf("[INFO] Extracted %zu cells (%d via character detection, %d via smart fallback)\n", 
           total_cells, (int)total_cells - filled_count, filled_count);
    
    if (debug_folder[0] != '\0')
    {
        char raw_folder[2048];
        snprintf(raw_folder, sizeof(raw_folder), "%s/RAW_CELLS", debug_folder);
        mkdir(raw_folder, 0755);
        
        printf("[INFO] Saving sample RAW cells (before reprocessing) to %s/\n", raw_folder);
        
        for (size_t i = 0; i < total_cells && i < 25; i++)
        {
            if (cells[i])
            {
                size_t row = i / (size_t)cols;
                size_t col = i % (size_t)cols;
                
                char path[2048];
                snprintf(path, sizeof(path),
                        "%s/RAW_%02zu_%02zu.png",
                        raw_folder, col, row);
                
                MagickWriteImage(cells[i], path);
            }
        }
    }
    
    reprocess_all_cells(cells, total_cells);
    
    if (debug_folder[0] != '\0')
    {
        char all_cells_folder[2048];
        snprintf(all_cells_folder, sizeof(all_cells_folder), "%s/ALL_CELLS", debug_folder);
        mkdir(all_cells_folder, 0755);
        
        printf("[INFO] Saving ALL %zu cells to %s/\n", total_cells, all_cells_folder);
        
        int saved_count = 0;
        int null_count = 0;
        
        for (size_t i = 0; i < total_cells; i++)
        {
            size_t row = i / (size_t)cols;
            size_t col = i % (size_t)cols;
            
            if (cells[i])
            {
                char path[2048];
                snprintf(path, sizeof(path),
                        "%s/GRID_%02zu_%02zu.png",
                        all_cells_folder, row, col);
                
                if (MagickWriteImage(cells[i], path) == MagickTrue)
                {
                    saved_count++;
                    if (saved_count % 50 == 0 || i == total_cells - 1)
                    {
                        printf("[DEBUG] Saved cell [c%zu,r%zu] (%d/%zu)\n",
                               col, row, saved_count, total_cells);
                    }
                }
            }
            else
            {
                null_count++;
                if (null_count <= 10)
                {
                    printf("[WARN] Cell [c%zu,r%zu] is NULL (failed to extract)\n", col, row);
                }
            }
        }
        
        printf("[INFO] ✓ Saved %d cells, %d cells were NULL\n", saved_count, null_count);
        
        size_t samples[] = {
            0, (size_t)(cols / 2), (size_t)(cols - 1),
            (size_t)((rows / 2) * cols),
            (size_t)((rows / 2) * cols + cols / 2),
            (size_t)((rows / 2) * cols + cols - 1),
            (size_t)((rows - 1) * cols),
            (size_t)((rows - 1) * cols + cols / 2),
            total_cells - 1
        };
        
        for (int i = 0; i < 9 && samples[i] < total_cells; i++)
        {
            if (cells[samples[i]])
            {
                size_t row = samples[i] / (size_t)cols;
                size_t col = samples[i] % (size_t)cols;
                
                char path[2048];
                snprintf(path, sizeof(path),
                        "%s/GRID_%02zu_%02zu.png",
                        debug_folder, row, col);
                
                MagickWriteImage(cells[samples[i]], path);
            }
        }
    }
    
    result.success = 1;
    result.cells = cells;
    result.rows = (size_t)rows;
    result.cols = (size_t)cols;
    result.total_cells = total_cells;
    
    return result;
}

void free_cells(CellExtractionResult *result)
{
    if (!result || !result->cells)
        return;
    
    for (size_t i = 0; i < result->total_cells; i++)
    {
        if (result->cells[i])
        {
            DestroyMagickWand(result->cells[i]);
            result->cells[i] = NULL;
        }
    }
    
    free(result->cells);
    result->cells = NULL;
    result->total_cells = 0;
    result->rows = 0;
    result->cols = 0;
    result->success = 0;
}