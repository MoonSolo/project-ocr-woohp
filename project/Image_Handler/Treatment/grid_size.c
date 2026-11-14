#include "grid_size.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_GRID_SIZE 5
#define MAX_GRID_SIZE 50
#define MIN_CHAR_SIZE 5
#define MAX_CHAR_SIZE 80
#define MIN_FILL_RATIO 0.75f

static char debug_folder[1024] = "";

void set_grid_size_debug_folder(const char *folder)
{
    snprintf(debug_folder, sizeof(debug_folder), "%s", folder);
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
    if (!s) return NULL;
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
        Point *new_data = (Point*)realloc(s->data,
                                         s->capacity * sizeof(Point));
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
    *x = s->data[s->size].x;
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
    if (!list)
        return NULL;
    
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
        if (!new_boxes)
            return;
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
        if (!pop(stack, &x, &y))
            break;
        
        if (x < 0 || x >= width || y < 0 || y >= height)
            continue;
        if (labels[y * width + x] != 0)
            continue;
        if (pixels[y * width + x] > 128)
            continue;
        
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

static BoxList* find_connected_components(unsigned char *pixels,
                                         int width, int height)
{
    BoxList *boxes = create_box_list();
    if (!boxes)
        return NULL;
    
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
            if (pixels[y * width + x] < 128 &&
                labels[y * width + x] == 0)
            {
                int min_x = width, max_x = 0;
                int min_y = height, max_y = 0;
                
                flood_fill_iterative(pixels, labels, width, height, x, y,
                                    current_label, &min_x, &max_x,
                                    &min_y, &max_y);
                
                int w = max_x - min_x + 1;
                int h = max_y - min_y + 1;
                
                if (w >= MIN_CHAR_SIZE && w <= MAX_CHAR_SIZE &&
                    h >= MIN_CHAR_SIZE && h <= MAX_CHAR_SIZE)
                {
                    float aspect = (float)w / (float)h;
                    
                    if (aspect > 0.2f && aspect < 5.0f)
                    {
                        BoundingBox box;
                        box.x = min_x;
                        box.y = min_y;
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

static int compare_int(const void *a, const void *b)
{
    return (*(int*)a - *(int*)b);
}

static int cluster_positions(int *positions, int count, int tolerance)
{
    if (count == 0)
        return 0;
    
    qsort(positions, count, sizeof(int), compare_int);
    
    int clusters = 1;
    int last = positions[0];
    
    for (int i = 1; i < count; i++)
    {
        if (positions[i] - last > tolerance)
        {
            clusters++;
            last = positions[i];
        }
    }
    
    return clusters;
}

static GridSizeResult analyze_character_grid(BoxList *boxes,
                                             int width, int height)
{
    GridSizeResult result = {false, 0, 0, 0.0, 0.0};
    
    if (boxes->count < MIN_GRID_SIZE * MIN_GRID_SIZE)
    {
        return result;
    }
    
    int avg_width = 0, avg_height = 0;
    for (int i = 0; i < boxes->count; i++)
    {
        avg_width += boxes->boxes[i].width;
        avg_height += boxes->boxes[i].height;
    }
    avg_width /= boxes->count;
    avg_height /= boxes->count;
    
    int *y_centers = (int*)malloc(boxes->count * sizeof(int));
    int *x_centers = (int*)malloc(boxes->count * sizeof(int));
    
    for (int i = 0; i < boxes->count; i++)
    {
        y_centers[i] = boxes->boxes[i].center_y;
        x_centers[i] = boxes->boxes[i].center_x;
    }
    
    int tolerance = (avg_width + avg_height) / 3;
    
    int rows = cluster_positions(y_centers, boxes->count, tolerance);
    int cols = cluster_positions(x_centers, boxes->count, tolerance);
    
    free(y_centers);
    free(x_centers);
    
    if (rows < MIN_GRID_SIZE || rows > MAX_GRID_SIZE ||
        cols < MIN_GRID_SIZE || cols > MAX_GRID_SIZE)
    {
        return result;
    }
    
    result.success = true;
    result.rows = rows;
    result.cols = cols;
    result.avg_cell_width = (double)width / (double)cols;
    result.avg_cell_height = (double)height / (double)rows;
    
    return result;
}

static void save_debug_overlay(MagickWand *original, BoxList *boxes,
                               size_t rows, size_t cols, const char *suffix)
{
    if (debug_folder[0] == '\0')
        return;
    
    MagickWand *debug_wand = CloneMagickWand(original);
    if (!debug_wand)
        return;
    
    DrawingWand *draw = NewDrawingWand();
    PixelWand *color = NewPixelWand();
    
    PixelSetColor(color, "lime");
    DrawSetStrokeColor(draw, color);
    DrawSetStrokeWidth(draw, 1.0);
    DrawSetFillOpacity(draw, 0.0);
    DrawSetStrokeOpacity(draw, 0.8);
    
    for (int i = 0; i < boxes->count; i++)
    {
        BoundingBox *box = &boxes->boxes[i];
        DrawRectangle(draw, box->x, box->y,
                     box->x + box->width, box->y + box->height);
    }
    
    MagickDrawImage(debug_wand, draw);
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/character_detection%s.png",
             debug_folder, suffix);
    MagickWriteImage(debug_wand, path);
    
    DestroyPixelWand(color);
    DestroyDrawingWand(draw);
    DestroyMagickWand(debug_wand);
}

typedef struct
{
    GridSizeResult result;
    int char_count;
    float fill_ratio;
    float score;
} Strategy;

GridSizeResult detect_grid_size(MagickWand *cropped_grid)
{
    GridSizeResult result = {false, 0, 0, 0.0, 0.0};
    
    if (!cropped_grid)
    {
        return result;
    }
    
    size_t width = MagickGetImageWidth(cropped_grid);
    size_t height = MagickGetImageHeight(cropped_grid);
    
    if (width < 50 || height < 50)
    {
        return result;
    }
    
    Strategy strategies[3];
    int strategy_count = 0;
    
    int adaptive_sizes[] = {11, 15, 19};
    
    for (int s = 0; s < 3; s++)
    {
        MagickWand *processed = CloneMagickWand(cropped_grid);
        if (!processed)
            continue;
        
        MagickTransformImageColorspace(processed, GRAYColorspace);
        MagickAdaptiveThresholdImage(processed, adaptive_sizes[s],
                                     adaptive_sizes[s], 0);
        
        unsigned char *pixels = (unsigned char*)malloc(width * height);
        if (!pixels)
        {
            DestroyMagickWand(processed);
            continue;
        }
        
        MagickExportImagePixels(processed, 0, 0, width, height, "I",
                               CharPixel, pixels);
        
        
        BoxList *boxes = find_connected_components(pixels, width, height);
        
        if (boxes && boxes->count > 0)
        {
            GridSizeResult candidate = analyze_character_grid(boxes,
                                                              width, height);
            
            if (candidate.success)
            {
                int expected = candidate.rows * candidate.cols;
                float fill = (float)boxes->count / (float)expected;
                
                float score = fill;
                if (fill >= 0.85f) score += 0.5f;
                if (fill >= 0.95f) score += 0.5f;
                
                strategies[strategy_count].result = candidate;
                strategies[strategy_count].char_count = boxes->count;
                strategies[strategy_count].fill_ratio = fill;
                strategies[strategy_count].score = score;
                
                if (s == 1 || strategy_count == 0)
                {
                    char suffix[32];
                    snprintf(suffix, sizeof(suffix), "_strategy%d", s + 1);
                    save_debug_overlay(cropped_grid, boxes,
                                      candidate.rows, candidate.cols, suffix);
                }
                
                strategy_count++;
            }
            
            free_box_list(boxes);
        }
        
        free(pixels);
        DestroyMagickWand(processed);
    }
    
    if (strategy_count == 0)
    {
        return result;
    }
    
    int best_idx = 0;
    float best_score = strategies[0].score;
    
    for (int i = 1; i < strategy_count; i++)
    {
        if (strategies[i].score > best_score)
        {
            best_score = strategies[i].score;
            best_idx = i;
        }
    }
    
    result = strategies[best_idx].result;
    
    return result;
}