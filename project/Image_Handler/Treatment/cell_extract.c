#include "cell_extract.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_CHAR_SIZE 5
#define MAX_CHAR_SIZE 80

static char debug_folder[1024] = "";

void set_cell_extract_debug_folder(const char *folder)
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

static BoxList* find_characters(unsigned char *pixels, int width, int height)
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

typedef struct
{
    int position;
    int count;
    int sum;
} Cluster;

static int find_grid_lines(BoxList *boxes, int dimension_getter(BoundingBox*),
                          int max_dim, Cluster **out_clusters)
{
    int *positions = (int*)malloc(boxes->count * sizeof(int));
    
    for (int i = 0; i < boxes->count; i++)
    {
        positions[i] = dimension_getter(&boxes->boxes[i]);
    }
    
    for (int i = 0; i < boxes->count - 1; i++)
    {
        for (int j = 0; j < boxes->count - i - 1; j++)
        {
            if (positions[j] > positions[j + 1])
            {
                int temp = positions[j];
                positions[j] = positions[j + 1];
                positions[j + 1] = temp;
            }
        }
    }
    
    int avg_char_size = 0;
    for (int i = 0; i < boxes->count; i++)
    {
        avg_char_size += boxes->boxes[i].width + boxes->boxes[i].height;
    }
    avg_char_size /= (boxes->count * 2);
    
    int tolerance = avg_char_size / 2;
    
    Cluster *clusters = (Cluster*)malloc(boxes->count * sizeof(Cluster));
    int cluster_count = 0;
    
    clusters[0].position = positions[0];
    clusters[0].sum = positions[0];
    clusters[0].count = 1;
    cluster_count = 1;
    
    for (int i = 1; i < boxes->count; i++)
    {
        if (positions[i] - clusters[cluster_count - 1].position <= tolerance)
        {
            clusters[cluster_count - 1].sum += positions[i];
            clusters[cluster_count - 1].count++;
            clusters[cluster_count - 1].position =
                clusters[cluster_count - 1].sum /
                clusters[cluster_count - 1].count;
        }
        else
        {
            clusters[cluster_count].position = positions[i];
            clusters[cluster_count].sum = positions[i];
            clusters[cluster_count].count = 1;
            cluster_count++;
        }
    }
    
    free(positions);
    *out_clusters = clusters;
    return cluster_count;
}

static int get_center_x(BoundingBox *box) { return box->center_x; }
static int get_center_y(BoundingBox *box) { return box->center_y; }

CellExtractionResult extract_cells(MagickWand *cropped_grid,
                                   size_t rows, size_t cols,
                                   double avg_cell_width,
                                   double avg_cell_height)
{
    CellExtractionResult result = {false, NULL, 0, 0, 0};
    
    if (!cropped_grid)
        return result;
    
    size_t width = MagickGetImageWidth(cropped_grid);
    size_t height = MagickGetImageHeight(cropped_grid);
    
    MagickWand *processed = CloneMagickWand(cropped_grid);
    if (!processed)
        return result;
    
    MagickTransformImageColorspace(processed, GRAYColorspace);
    MagickAdaptiveThresholdImage(processed, 15, 15, 0);
    
    unsigned char *pixels = (unsigned char*)malloc(width * height);
    if (!pixels)
    {
        DestroyMagickWand(processed);
        return result;
    }
    
    MagickExportImagePixels(processed, 0, 0, width, height, "I",
                           CharPixel, pixels);
    
    BoxList *boxes = find_characters(pixels, width, height);
    
    if (!boxes || boxes->count < rows * cols * 0.7)
    {
        if (boxes)
            free_box_list(boxes);
        free(pixels);
        DestroyMagickWand(processed);
        return result;
    }
    
    Cluster *x_clusters = NULL;
    Cluster *y_clusters = NULL;
    
    int x_cluster_count = find_grid_lines(boxes, get_center_x, width,
                                         &x_clusters);
    int y_cluster_count = find_grid_lines(boxes, get_center_y, height,
                                         &y_clusters);
    
    int *col_positions = (int*)malloc((cols + 1) * sizeof(int));
    int *row_positions = (int*)malloc((rows + 1) * sizeof(int));
    
    for (size_t i = 0; i <= cols; i++)
    {
        col_positions[i] = (int)(i * avg_cell_width);
    }
    
    for (size_t i = 0; i <= rows; i++)
    {
        row_positions[i] = (int)(i * avg_cell_height);
    }
    
    if (x_cluster_count == cols && y_cluster_count == rows)
    {
        for (size_t i = 0; i < cols; i++)
        {
            int spacing = (i == cols - 1) ?
                         (width - x_clusters[i].position) :
                         (x_clusters[i + 1].position - x_clusters[i].position);
            
            col_positions[i] = x_clusters[i].position - spacing / 2;
        }
        col_positions[cols] = width;
        
        for (size_t i = 0; i < rows; i++)
        {
            int spacing = (i == rows - 1) ?
                         (height - y_clusters[i].position) :
                         (y_clusters[i + 1].position - y_clusters[i].position);
            
            row_positions[i] = y_clusters[i].position - spacing / 2;
        }
        row_positions[rows] = height;
    }
    
    size_t total_cells = rows * cols;
    MagickWand **cells = (MagickWand**)malloc(total_cells *
                                              sizeof(MagickWand*));
    if (!cells)
    {
        free(col_positions);
        free(row_positions);
        free(x_clusters);
        free(y_clusters);
        free(pixels);
        free_box_list(boxes);
        DestroyMagickWand(processed);
        return result;
    }
    
    for (size_t row = 0; row < rows; row++)
    {
        for (size_t col = 0; col < cols; col++)
        {
            size_t idx = row * cols + col;
            
            int cell_x = col_positions[col];
            int cell_y = row_positions[row];
            int cell_w = col_positions[col + 1] - cell_x;
            int cell_h = row_positions[row + 1] - cell_y;
            
            if (cell_x < 0) cell_x = 0;
            if (cell_y < 0) cell_y = 0;
            if (cell_x + cell_w > (int)width) cell_w = width - cell_x;
            if (cell_y + cell_h > (int)height) cell_h = height - cell_y;
            
            MagickWand *cell = CloneMagickWand(cropped_grid);
            
            if (cell && MagickCropImage(cell, cell_w, cell_h,
                                       cell_x, cell_y) != MagickFalse)
            {
                MagickResetImagePage(cell, "");
                cells[idx] = cell;
            }
            else
            {
                if (cell)
                    DestroyMagickWand(cell);
                cells[idx] = NULL;
            }
        }
    }
    
    if (debug_folder[0] != '\0')
    {
        size_t samples[] = {0, cols / 2, cols - 1,
                          (rows / 2) * cols, (rows / 2) * cols + cols / 2,
                          (rows / 2) * cols + cols - 1,
                          (rows - 1) * cols, (rows - 1) * cols + cols / 2,
                          total_cells - 1};
        
        for (int i = 0; i < 9 && samples[i] < total_cells; i++)
        {
            if (cells[samples[i]])
            {
                size_t row = samples[i] / cols;
                size_t col = samples[i] % cols;
                
                char path[1024];
                snprintf(path, sizeof(path),
                        "%s/cell_sample_r%zu_c%zu.png",
                        debug_folder, row, col);
                MagickWriteImage(cells[samples[i]], path);
            }
        }
    }
    
    free(col_positions);
    free(row_positions);
    free(x_clusters);
    free(y_clusters);
    free(pixels);
    free_box_list(boxes);
    DestroyMagickWand(processed);
    
    result.success = true;
    result.cells = cells;
    result.rows = rows;
    result.cols = cols;
    result.total_cells = total_cells;
    
    return result;
}

void free_cell_extraction_result(CellExtractionResult *result)
{
    if (result && result->cells)
    {
        for (size_t i = 0; i < result->total_cells; i++)
        {
            if (result->cells[i])
                DestroyMagickWand(result->cells[i]);
        }
        free(result->cells);
        result->cells = NULL;
    }
}