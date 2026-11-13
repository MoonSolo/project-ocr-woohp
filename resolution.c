#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <math.h>
#include <MagickWand/MagickWand.h>

#include "solver.h"
#include "grid_detect.h"
#include "grid_size.h"

#define OFFSET_Y  2      
#define OFFSET_X  1
#define EXTENSION 0.25   
#define THICKNESS 0.35   
#define ALPHA     0.35   
#define CENTER_INSET 0.10 

void solve_and_mark(const char *mots_path, const char *grid_path, const char *image_path)
{
    int dimensions[2] = {0, 0};
    if (!grid_path) errx(1, "Error: grid_path is NULL");
    
    count_lines_columns(grid_path, dimensions);
    int lines = dimensions[0];
    int columns = dimensions[1];

    FILE *check_fp = fopen(grid_path, "r");
    if (check_fp) {
        fseek(check_fp, -1, SEEK_END);
        if (fgetc(check_fp) == '\n') {
             if (lines == columns + 1) {
                printf("Notice: Detected potential trailing newline count. Reducing lines from %d to %d.\n", lines, lines - 1);
                lines--; 
            }
        }
        fclose(check_fp);
    }

    if (lines == 0 || columns == 0) errx(1, "Empty grid.");
    char crossword[lines][columns];
    draw_crossword(grid_path, lines, columns, crossword);

    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();
    if (MagickReadImage(wand, image_path) == MagickFalse) {
        DestroyMagickWand(wand);
        MagickWandTerminus();
        errx(1, "Error opening image %s", image_path);
    }

    GridDetectionResult grid_pos = detect_grid(wand);
    if (!grid_pos.detected) {
        grid_pos.x = 0; grid_pos.y = 0;
        grid_pos.width = MagickGetImageWidth(wand);
        grid_pos.height = MagickGetImageHeight(wand);
    } 

    double raw_cell_width = (double)grid_pos.width / (double)columns;
    double raw_cell_height = (double)grid_pos.height / (double)lines;

    double start_x = grid_pos.x + (raw_cell_width * CENTER_INSET) + OFFSET_X;
    double start_y = grid_pos.y + (raw_cell_height * CENTER_INSET) + OFFSET_Y; 

    double effective_width = grid_pos.width - (raw_cell_width * CENTER_INSET * 2);
    double effective_height = grid_pos.height - (raw_cell_height * CENTER_INSET * 2);

    double cell_width = effective_width / (double)columns;
    double cell_height = effective_height / (double)lines;

    printf("Grid: %dx%d, Cell: %.2fx%.2f\n", columns, lines, cell_width, cell_height);

    DrawingWand *draw = NewDrawingWand();
    PixelWand *stroke_color = NewPixelWand();
    PixelSetColor(stroke_color, "rgba(255, 0, 0, 0.35)"); 
    DrawSetStrokeColor(draw, stroke_color);
    
    double stroke_w = (cell_width < cell_height ? cell_width : cell_height) * THICKNESS;
    DrawSetStrokeWidth(draw, stroke_w);
    DrawSetStrokeLineCap(draw, RoundCap);
    DrawSetStrokeLineJoin(draw, RoundJoin);

    FILE *fp = fopen(mots_path, "r");
    if (!fp) errx(1, "Cannot open word file");

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        buffer[strcspn(buffer, "\r")] = '\0';
        if (strlen(buffer) == 0) continue;

        int coords[2][2] = {{-1, -1}, {-1, -1}};
        search_word(buffer, lines, columns, coords, crossword);

        if (coords[1][0] != -1) {
            int x1 = coords[0][0];
            int y1 = coords[0][1];
            int x2 = coords[1][1];
            int y2 = coords[1][0];

            double px1 = start_x + (x1 * cell_width) + (cell_width / 2.0);
            double py1 = start_y + (y1 * cell_height) + (cell_height / 2.0);
            double px2 = start_x + (x2 * cell_width) + (cell_width / 2.0);
            double py2 = start_y + (y2 * cell_height) + (cell_height / 2.0);

            double dx = px2 - px1;
            double dy = py2 - py1;
            double len = sqrt(dx*dx + dy*dy);
            
            if (len > 0.001) {
                dx /= len;
                dy /= len;
                double ext_len = (cell_width < cell_height ? cell_width : cell_height) * EXTENSION;
                px1 -= dx * ext_len;
                py1 -= dy * ext_len;
                px2 += dx * ext_len;
                py2 += dy * ext_len;
            }

            DrawLine(draw, px1, py1, px2, py2);
            printf("Marked: %s\n", buffer);
        }
    }
    fclose(fp);

    MagickDrawImage(wand, draw);
    char output_filename[1024];
    const char *ext = strrchr(image_path, '.');
    if (ext) {
        int base_len = ext - image_path;
        snprintf(output_filename, sizeof(output_filename), "%.*s_solved%s", base_len, image_path, ext);
    } else {
        snprintf(output_filename, sizeof(output_filename), "%s_solved.jpg", image_path);
    }
    MagickWriteImage(wand, output_filename);
    printf("Saved to: %s\n", output_filename);

    DestroyDrawingWand(draw);
    DestroyPixelWand(stroke_color);
    DestroyMagickWand(wand);
    MagickWandTerminus();
}

int main(int argc, char *argv[]) {
    if (argc < 4) return 1;
    solve_and_mark(argv[2], argv[1], argv[3]);
    return 0;
}

