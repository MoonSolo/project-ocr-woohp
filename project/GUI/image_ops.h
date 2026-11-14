#ifndef IMAGE_OPS_H
#define IMAGE_OPS_H

#include <gtk/gtk.h>
#include <string.h>

// Creates and returns a "OPEN IMAGE FILE" button. Will serve for file explorer.
GtkWidget* create_open_file_button(void);
// Centering IN the parent container.
void resize_button_center(GtkWidget *button, int width, int height);
char* open_file_chooser(GtkWidget *button);
void clear_window(GtkWidget *window);
GtkWidget* create_clear_button(void);
GtkWidget *create_rotation_controls(GtkWidget *overlay, const char *current_file);
void replace_image_in_overlay(GtkWidget *overlay, const char *file);
GtkWidget *create_process_button(void);

#endif
