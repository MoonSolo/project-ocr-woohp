#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include "rotate.h"

/* The main goal of this file is to perform a MANUAL rotation of the image.
 * It may be called by another function since it is able to rotate an image and create
 * a copy of it.
 * OUTPUT NAME : "<original_name>_r.bmp"
 *
 * errors handling are explicit.
*/

///////////////////////////////////////////////////////////////////////////////
/* SOME ISSUES OCCURED WHEN WE TRIED TO IMPORT THE CAIRO GDK. WE MUST DECLARE
 * THE FUNCTIONS MANUALLY TO PRESERVE STRUCTURE.
*/

GdkPixbuf *gdk_pixbuf_get_from_surface(cairo_surface_t *surface,
                                       gint src_x,
                                       gint src_y,
                                       gint width,
                                       gint height);

void gdk_cairo_set_source_pixbuf(cairo_t *cr,
                                 const GdkPixbuf *pixbuf,
                                 double pixbuf_x,
                                 double pixbuf_y);


///////////////////////////////////////////////////////////////////////////////

static char *output_naming(const char *input_path) // renames the input file. private.
{
    const char *base = strrchr(input_path, '/');
    base = base ? base + 1 : input_path;

    char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);

    const char *directory = "../WORKING/";
    size_t dir_len = strlen(directory);

    char *output = malloc(dir_len + len + 7); // + 7 => "_r.bmp\0"
    if (!output)
        return NULL;

    snprintf(output, dir_len + len + 7,
             "%s%.*s_r.bmp", directory, (int)len, base);
    return output;
}

int rotate_image(const char *input_path, double degrees)
{
    GError *error = NULL;

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(input_path, &error);
    if (!pixbuf)
    {
        g_printerr("Error loading image '%s': %s\n", input_path, error->message);
        g_clear_error(&error);
        return 1;
    }

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);

    double radians = degrees * (M_PI / 180.0);

    double sin_theta = fabs(sin(radians));
    double cos_theta = fabs(cos(radians));
    int new_width = (int)(width * cos_theta + height * sin_theta);
    int new_height = (int)(width * sin_theta + height * cos_theta);

    // same as SDL !
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
    cairo_t *cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_translate(cr, new_width / 2.0, new_height / 2.0);
    cairo_rotate(cr, radians);
    cairo_translate(cr, -width / 2.0, -height / 2.0);

    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);

    GdkPixbuf *rotated =
        gdk_pixbuf_get_from_surface(surface, 0, 0, new_width, new_height);

    // saving as .bmp for conventional format for splitter and imagemagick.
    char *output_name = output_naming(input_path);
    if (!output_name)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_object_unref(pixbuf);
        g_object_unref(rotated);
        return 1;
    }

    if (!gdk_pixbuf_save(rotated, output_name, "bmp", &error, NULL))
    {
        g_printerr("Error saving image '%s': %s\n", output_name, error->message);
        g_clear_error(&error);
        free(output_name);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_object_unref(pixbuf);
        g_object_unref(rotated);
        return 1;
    }

    printf("[ INFO ] Saved rotated image: %s\n", output_name);

    free(output_name);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(pixbuf);
    g_object_unref(rotated);

    return 0;
}

/*
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "[ ERROR ] Invalid call of the function.\n"
                        "Usage: %s <image> <degrees>\n", argv[0]);
        return 1;
    }

    const char *image_path = argv[1];
    double degrees = atof(argv[2]);

    return rotate_image(image_path, degrees);
}
*/
