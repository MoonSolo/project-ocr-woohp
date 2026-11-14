#include "grid_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int start;
    int size;
} Gap;

static char debug_folder[1024] = "";

void set_grid_detection_debug_folder(const char *folder)
{
    snprintf(debug_folder, sizeof(debug_folder), "%s", folder);
}

static void save_histogram_image(int *hist, int len, const char *name,
                                int threshold)
{
    if (debug_folder[0] == '\0')
        return;

    MagickWand *wand = NewMagickWand();
    PixelWand *bg_color = NewPixelWand();
    PixelWand *bar_color = NewPixelWand();
    PixelWand *thresh_color = NewPixelWand();
    
    PixelSetColor(bg_color, "white");
    PixelSetColor(bar_color, "rgb(50,50,200)");
    PixelSetColor(thresh_color, "red");
    
    int width = (len > 1000) ? 1000 : len;
    int height = 400;
    
    MagickNewImage(wand, width, height, bg_color);
    
    int max_val = 0;
    for (int i = 0; i < len; i++)
    {
        if (hist[i] > max_val)
            max_val = hist[i];
    }
    
    if (max_val == 0)
        max_val = 1;
    
    DrawingWand *draw = NewDrawingWand();
    DrawSetFillColor(draw, bar_color);
    DrawSetStrokeColor(draw, bar_color);
    DrawSetStrokeWidth(draw, 1.0);
    
    int step = (len > width) ? ((len + width - 1) / width) : 1;
    
    for (int i = 0; i < len; i += step)
    {
        int avg = 0, count = 0;
        for (int j = i; j < i + step && j < len; j++)
        {
            avg += hist[j];
            count++;
        }
        if (count > 0)
            avg /= count;
        
        int x = i / step;
        if (x >= width)
            break;
            
        int bar_height = (avg * (height - 40)) / max_val;
        if (bar_height > 0)
        {
            DrawRectangle(draw, x, height - bar_height - 20,
                         x + 1, height - 20);
        }
    }
    
    DrawSetFillColor(draw, thresh_color);
    DrawSetStrokeColor(draw, thresh_color);
    int thresh_y = height - 20 - ((threshold * (height - 40)) / max_val);
    DrawLine(draw, 0, thresh_y, width, thresh_y);
    
    MagickDrawImage(wand, draw);
    
    char filename[2048];
    snprintf(filename, sizeof(filename), "%s/histogram_%s.png",
             debug_folder, name);
    MagickWriteImage(wand, filename);
    
    DestroyDrawingWand(draw);
    DestroyPixelWand(thresh_color);
    DestroyPixelWand(bar_color);
    DestroyPixelWand(bg_color);
    DestroyMagickWand(wand);
}

static int find_significant_gaps(int *hist, int len, int threshold,
                                Gap *gaps, int max_gaps)
{
    int gap_count = 0;
    int gap_start = -1;
    
    for (int i = 0; i < len && gap_count < max_gaps; i++)
    {
        if (hist[i] <= threshold)
        {
            if (gap_start == -1)
                gap_start = i;
        }
        else
        {
            if (gap_start != -1)
            {
                int gap_size = i - gap_start;
                if (gap_size >= MIN_SIGNIFICANT_GAP)
                {
                    gaps[gap_count].start = gap_start;
                    gaps[gap_count].size = gap_size;
                    gap_count++;
                }
                gap_start = -1;
            }
        }
    }
    
    if (gap_start != -1 &&
        len - gap_start >= MIN_SIGNIFICANT_GAP &&
        gap_count < max_gaps)
    {
        int gap_size = len - gap_start;
        gaps[gap_count].start = gap_start;
        gaps[gap_count]. size = gap_size;
        gap_count++;
    }
    
    return gap_count;
}

static int segment_with_gaps(unsigned char *pixels, int width, int height, 
                            Rectangle *rects, int max_rects,
                            int **h_hist_out, int **v_hist_out)
{
    int rect_count = 0;
    
    int *v_hist = (int*)calloc(width, sizeof(int));
    int *h_hist = (int*)calloc(height, sizeof(int));
    
    if (!v_hist || !h_hist)
    {
        free(v_hist);
        free(h_hist);
        return 0;
    }
    
    compute_vertical_histogram(pixels, width, height, v_hist);
    compute_horizontal_histogram(pixels, width, height, h_hist);
    
    *v_hist_out = v_hist;
    *h_hist_out = h_hist;
    
    int v_threshold = height * 0.03;
    int h_threshold = width * 0.03;
    
    print_histogram("Vertical (columns)", v_hist, width, v_threshold);
    print_histogram("Horizontal (rows)", h_hist, height, h_threshold);
    
    save_histogram_image(v_hist, width, "vertical", v_threshold);
    save_histogram_image(h_hist, height, "horizontal", h_threshold);
    
    Gap v_gaps[10];
    int v_gap_count = find_significant_gaps(v_hist, width, v_threshold,
                                           v_gaps, 10);
    
    Gap h_gaps[10];
    int h_gap_count = find_significant_gaps(h_hist, height, h_threshold,
                                           h_gaps, 10);
    
    if (v_gap_count > 0 || h_gap_count > 0)
    {
        int y_splits[12], x_splits[12];
        int y_count = 0, x_count = 0;
        
        y_splits[y_count++] = 0;
        for (int i = 0; i < h_gap_count; i++)
        {
            y_splits[y_count++] = h_gaps[i].start;
            y_splits[y_count++] = h_gaps[i].start + h_gaps[i].size;
        }
        y_splits[y_count++] = height;
        
        x_splits[x_count++] = 0;
        for (int i = 0; i < v_gap_count; i++)
        {
            x_splits[x_count++] = v_gaps[i].start;
            x_splits[x_count++] = v_gaps[i].start + v_gaps[i].size;
        }
        x_splits[x_count++] = width;
        
        for (int yi = 0; yi < y_count - 1 && rect_count < max_rects; yi++)
        {
            for (int xi = 0;
                 xi < x_count - 1 && rect_count < max_rects; xi++)
            {
                Rectangle region = extract_bounds(pixels, width, height, 
                                                 x_splits[xi],
                                                 x_splits[xi + 1],
                                                 y_splits[yi],
                                                 y_splits[yi + 1]);
                
                if (region.width >= MIN_RECT_SIZE &&
                    region.height >= MIN_RECT_SIZE)
                {
                    rects[rect_count++] = region;
                }
            }
        }
    }
    
    return rect_count;
}

static int fallback_sliding_window(unsigned char *pixels, int width,
                                   int height, Rectangle *rects,
                                   int max_rects)
{
    int rect_count = 0;
    float best_score = 0.0f;
    Rectangle best = {0};
    
    float aspects[] = {0.7f, 0.85f, 1.0f, 1.15f, 1.4f};
    
    for (int a = 0; a < 5; a++)
    {
        float aspect = aspects[a];
        
        for (int h = 150; h <= height && h <= 450; h += 20)
        {
            int w = (int)(h * aspect);
            if (w < 150 || w > width)
                continue;
            
            for (int y = 0; y <= height - h; y += 15)
            {
                for (int x = 0; x <= width - w; x += 15)
                {
                    Rectangle region = extract_bounds(pixels, width, height,
                                                     x, x + w, y, y + h);
                    
                    if (region. area > 0)
                    {
                        float score = score_as_grid(region, width, height);
                        
                        if (score > best_score)
                        {
                            best_score = score;
                            best = region;
                        }
                    }
                }
            }
        }
    }
    
    if (best.area > 0 && rect_count < max_rects)
    {
        rects[rect_count++] = best;
    }
    
    return rect_count;
}

static void trim_uniform_borders(MagickWand *cropped, int *x_offset,
                                int *y_offset, int *width_out, int *height_out)
{
    size_t width = MagickGetImageWidth(cropped);
    size_t height = MagickGetImageHeight(cropped);
    
    if (width < 50 || height < 50)
        return;
    
    MagickWand *temp = CloneMagickWand(cropped);
    if (! temp)
        return;
    
    MagickTransformImageColorspace(temp, GRAYColorspace);
    MagickThresholdImage(temp, QuantumRange * 0.5);
    
    unsigned char *pixels = (unsigned char*)malloc(width * height);
    if (!pixels)
    {
        DestroyMagickWand(temp);
        return;
    }
    
    MagickExportImagePixels(temp, 0, 0, width, height, "I",
                           CharPixel, pixels);
    DestroyMagickWand(temp);
    
    int *row_density = (int*)calloc(height, sizeof(int));
    int *col_density = (int*)calloc(width, sizeof(int));
    
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            if (pixels[y * width + x] < 128)
            {
                row_density[y]++;
                col_density[x]++;
            }
        }
    }
    
    int center_row_start = height / 3;
    int center_row_end = height * 2 / 3;
    int center_col_start = width / 3;
    int center_col_end = width * 2 / 3;
    
    int center_row_avg = 0;
    for (int y = center_row_start; y < center_row_end; y++)
        center_row_avg += row_density[y];
    center_row_avg /= (center_row_end - center_row_start);
    
    int center_col_avg = 0;
    for (int x = center_col_start; x < center_col_end; x++)
        center_col_avg += col_density[x];
    center_col_avg /= (center_col_end - center_col_start);
    
    int trim_top = 0, trim_bottom = 0, trim_left = 0, trim_right = 0;
    
    int border_threshold_row = center_row_avg * 1.8;
    int border_threshold_col = center_col_avg * 1.8;
    
    for (int y = 0; y < (int)height / 4; y++)
    {
        if (row_density[y] > border_threshold_row)
            trim_top = y + 1;
        else if (trim_top > 0)
            break;
    }
    
    for (int y = height - 1; y >= (int)height * 3 / 4; y--)
    {
        if (row_density[y] > border_threshold_row)
            trim_bottom = height - y;
        else if (trim_bottom > 0)
            break;
    }
    
    for (int x = 0; x < (int)width / 4; x++)
    {
        if (col_density[x] > border_threshold_col)
            trim_left = x + 1;
        else if (trim_left > 0)
            break;
    }
    
    for (int x = width - 1; x >= (int)width * 3 / 4; x--)
    {
        if (col_density[x] > border_threshold_col)
            trim_right = width - x;
        else if (trim_right > 0)
            break;
    }
    
    free(row_density);
    free(col_density);
    free(pixels);
    
    int total_trim = trim_top + trim_bottom + trim_left + trim_right;
    if (total_trim < 10)
        return;
    
    int new_width = width - trim_left - trim_right;
    int new_height = height - trim_top - trim_bottom;
    
    if (new_width < 100 || new_height < 100)
        return;
    
    int margin_left = (trim_left > 5) ? ((trim_left * 8) / 10) : 0;
    int margin_right = (trim_right > 5) ? ((trim_right * 8) / 10) : 0;
    int margin_top = (trim_top > 5) ? ((trim_top * 8) / 10) : 0;
    int margin_bottom = (trim_bottom > 5) ? ((trim_bottom * 8) / 10) : 0;
    
    if (trim_left > 0 && margin_left < 4) margin_left = 4;
    if (trim_right > 0 && margin_right < 4) margin_right = 4;
    if (trim_top > 0 && margin_top < 4) margin_top = 4;
    if (trim_bottom > 0 && margin_bottom < 4) margin_bottom = 4;
    
    trim_left -= margin_left;
    trim_right -= margin_right;
    trim_top -= margin_top;
    trim_bottom -= margin_bottom;
    
    if (trim_left < 0) trim_left = 0;
    if (trim_right < 0) trim_right = 0;
    if (trim_top < 0) trim_top = 0;
    if (trim_bottom < 0) trim_bottom = 0;
    
    new_width = width - trim_left - trim_right;
    new_height = height - trim_top - trim_bottom;
    
    *x_offset += trim_left;
    *y_offset += trim_top;
    *width_out = new_width;
    *height_out = new_height;
}

GridDetectionResult detect_grid(MagickWand *input)
{
    GridDetectionResult result = {false, 0, 0, 0, 0};
    
    if (!input)
    {
        fprintf(stderr, "[GRID] ERROR: NULL input\n");
        return result;
    }
    
    size_t width = MagickGetImageWidth(input);
    size_t height = MagickGetImageHeight(input);
    
    MagickWand *gray = CloneMagickWand(input);
    MagickTransformImageColorspace(gray, GRAYColorspace);
    MagickNormalizeImage(gray);
    
    unsigned char *pixels = (unsigned char*)malloc(width * height);
    if (!pixels)
    {
        DestroyMagickWand(gray);
        return result;
    }
    
    MagickExportImagePixels(gray, 0, 0, width, height, "I",
                           CharPixel, pixels);
    DestroyMagickWand(gray);
    
    int *h_hist = NULL, *v_hist = NULL;
    Rectangle rects[MAX_RECTANGLES];
    int rect_count = segment_with_gaps(pixels, width, height, rects,
                                      MAX_RECTANGLES, &h_hist, &v_hist);
    
    if (rect_count > 0 && h_hist && v_hist)
    {
        int total_h = 0;
        for (int i = 0; i < (int)height; i++)
            total_h += h_hist[i];
        int avg_h = total_h / height;
        
        for (int i = 0; i < rect_count; i++)
        {
            rects[i] = trim_edge_decorations(rects[i], h_hist, v_hist,
                                            (int)width, (int)height, avg_h);
        }
    }
    
    if (rect_count == 0)
    {
        rect_count = fallback_sliding_window(pixels, width, height,
                                            rects, MAX_RECTANGLES);
    }
    
    free(pixels);
    free(h_hist);
    free(v_hist);
    
    if (rect_count == 0)
    {
        return result;
    }
    
    int best_idx = -1;
    float best_score = 0.0f;
    
    for (int i = 0; i < rect_count; i++)
    {
        float score = score_as_grid(rects[i], width, height);
        
        if (score > best_score)
        {
            best_score = score;
            best_idx = i;
        }
    }
    
    if (best_idx == -1 || best_score < 0.25f)
    {
        return result;
    }
    
    Rectangle grid = rects[best_idx];
    
    int final_x = (grid.x > MARGIN) ? (grid.x - MARGIN) : 0;
    int final_y = (grid.y > MARGIN) ? (grid.y - MARGIN) : 0;
    int final_w = grid.width + 2 * MARGIN;
    int final_h = grid.height + 2 * MARGIN;
    
    if (final_x + final_w > (int)width)
        final_w = width - final_x;
    if (final_y + final_h > (int)height)
        final_h = height - final_y;
    
    MagickWand *cropped = CloneMagickWand(input);
    if (! cropped ||
        MagickCropImage(cropped, final_w, final_h,
                       final_x, final_y) == MagickFalse)
    {
        if (cropped)
            DestroyMagickWand(cropped);
        return result;
    }
    
    MagickResetImagePage(cropped, "");
    
    int refined_x = final_x;
    int refined_y = final_y;
    int refined_w = final_w;
    int refined_h = final_h;
    
    trim_uniform_borders(cropped, &refined_x, &refined_y, &refined_w, &refined_h);
    
    if (refined_x != final_x || refined_y != final_y || 
        refined_w != final_w || refined_h != refined_h)
    {
        DestroyMagickWand(cropped);
        
        cropped = CloneMagickWand(input);
        if (cropped)
        {
            MagickCropImage(cropped, refined_w, refined_h, refined_x, refined_y);
            MagickResetImagePage(cropped, "");
        }
        
        final_x = refined_x;
        final_y = refined_y;
        final_w = refined_w;
        final_h = refined_h;
    }

    if (debug_folder[0] != '\0')
    {
        char path[2048];
        snprintf(path, sizeof(path), "%s/grid_cropped.png", debug_folder);
        if (cropped)
            MagickWriteImage(cropped, path);
    }
    
    if (cropped)
        DestroyMagickWand(cropped);
    
    result.detected = true;
    result.x = final_x;
    result.y = final_y;
    result.width = final_w;
    result.height = final_h;
    
    return result;
}