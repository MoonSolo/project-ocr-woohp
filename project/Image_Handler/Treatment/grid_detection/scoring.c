#include "grid_detect.h"
#include <math.h>

Rectangle extract_bounds(unsigned char *pixels, int width, int height,
                        int x_min, int x_max, int y_min, int y_max)
{
    Rectangle rect = {0, 0, 0, 0, 0, 0.0f};
    
    int min_x = x_max, max_x = x_min;
    int min_y = y_max, max_y = y_min;
    int dark_pixels = 0;
    
    for (int y = y_min; y < y_max && y < height; y++)
    {
        for (int x = x_min; x < x_max && x < width; x++)
        {
            if (pixels[y * width + x] < 128)
            {
                if (x < min_x)
                    min_x = x;
                if (x > max_x)
                    max_x = x;
                if (y < min_y)
                    min_y = y;
                if (y > max_y)
                    max_y = y;
                dark_pixels++;
            }
        }
    }
    
    if (max_x >= min_x && max_y >= min_y)
    {
        rect.x = min_x;
        rect.y = min_y;
        rect.width = max_x - min_x + 1;
        rect.height = max_y - min_y + 1;
        rect.area = rect.width * rect.height;
        if (rect.area > 0)
        {
            rect.density = (float)dark_pixels / (float)rect.area;
        }
    }
    
    return rect;
}

float score_as_grid(Rectangle rect, int image_width, int image_height)
{
    if (rect.area == 0)
        return 0.0f;
    if (rect.width < MIN_RECT_SIZE || rect.height < MIN_RECT_SIZE)
        return 0.0f;
    
    float aspect = (float)rect.width / (float)rect.height;
    float aspect_score = 0.0f;
    if (aspect >= 0.5f && aspect <= 2.0f)
    {
        float deviation = fabs(1.0f - aspect);
        if (deviation <= 0.5f)
        {
            aspect_score = 1.0f - (deviation * 0.4f);
        }
        else
        {
            aspect_score = 0.8f - ((deviation - 0.5f) * 0.5f);
        }
        if (aspect_score < 0.3f)
            aspect_score = 0.3f;
    }
    
    float density_score = 0.0f;
    if (rect.density >= 0.06f && rect.density <= 0.35f)
    {
        density_score = 1.0f;
    }
    else if (rect.density >= 0.04f && rect.density < 0.06f)
    {
        density_score = (rect.density - 0.04f) / 0.02f * 0.8f;
    }
    else if (rect.density > 0.35f && rect.density <= 0.45f)
    {
        density_score = 1.0f - (rect.density - 0.35f) / 0.10f * 0.5f;
    }
    
    float size_ratio = (float)rect.area /
                      (float)(image_width * image_height);
    float size_score = 0.0f;
    
    if (size_ratio >= 0.30f && size_ratio <= 0.70f)
    {
        size_score = 1.0f;
    }
    else if (size_ratio >= 0.20f && size_ratio < 0.30f)
    {
        size_score = 0.6f + (size_ratio - 0.20f) / 0.10f * 0.4f;
    }
    else if (size_ratio >= 0.15f && size_ratio < 0.20f)
    {
        size_score = 0.4f + (size_ratio - 0.15f) / 0.05f * 0.2f;
    }
    else if (size_ratio > 0.70f && size_ratio <= 0.85f)
    {
        size_score = 1.0f - (size_ratio - 0.70f) / 0.15f * 0.3f;
    }
    
    float total = aspect_score * 0.20f +
                 density_score * 0.40f +
                 size_score * 0.40f;
    return total;
}