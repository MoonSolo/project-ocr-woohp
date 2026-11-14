#include "image_ops.h"
#include "rotate.h"

GtkWidget* create_open_file_button(void)
{
    GtkWidget *button;
    button = gtk_button_new_with_label("OPEN IMAGE FILE");

    // For now we just return it; signals will be connected in gui.c
    return button;
}

void resize_button_center(GtkWidget *button, int width, int height)
{
    gtk_widget_set_size_request(button, width, height);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
}

char * open_file_chooser(GtkWidget *button)
{
    GtkWidget *dialog;
    char *file = NULL;

    dialog = gtk_file_chooser_dialog_new(
                    "Open Image File",
                    GTK_WINDOW(gtk_widget_get_toplevel(button)),
                    GTK_FILE_CHOOSER_ACTION_OPEN,
                    "_Cancel", GTK_RESPONSE_CANCEL,
                    "_Open", GTK_RESPONSE_ACCEPT,
                    NULL
    );

    // Apply file filters "si tu charge un .mp4 [...]"
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image files");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), ".");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (temp)
        {
            file = g_strdup(temp);
            g_free(temp);
        }
    }
    gtk_widget_destroy(dialog);
    return file;
}

void clear_window(GtkWidget *window)
{
    if (!GTK_IS_WINDOW(window))
        return;

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(window));
    if (!child)
        return;

    if (GTK_IS_CONTAINER(child))
    {
        GList *children = gtk_container_get_children(GTK_CONTAINER(child));
        for (GList *iter = children; iter != NULL; iter = iter->next)
            gtk_widget_destroy(GTK_WIDGET(iter->data));

        g_list_free(children);
    }

    gtk_container_remove(GTK_CONTAINER(window), child);
}


GtkWidget* create_clear_button(void)
{
    GtkWidget *button = gtk_button_new_with_label("CLEAR");

    // Align to top-left
    gtk_widget_set_halign(button, GTK_ALIGN_START);
    gtk_widget_set_valign(button, GTK_ALIGN_START);

    // Small margin from the edges
    gtk_widget_set_margin_top(button, 5);
    gtk_widget_set_margin_start(button, 5);

    return button;
}




/////////////////////////////////// TESTING ///////////////////////////////////

typedef struct {
    GtkWidget *overlay;
    char *current_file;
    double current_angle;
} RotationContext;

static void on_rotate_button_clicked(GtkWidget *button, gpointer user_data)
{
    RotationContext *ctx = (RotationContext *)user_data;
    const char *label = gtk_button_get_label(GTK_BUTTON(button));
    double delta = (strcmp(label, "↻") == 0) ? 1.0 : -1.0;

    ctx->current_angle += delta;

    const char *base = strrchr(ctx->current_file, '/');
    base = base ? base + 1 : ctx->current_file;

    char output_path[512];
    const char *dot = strrchr(base, '.');
    size_t name_len = dot ? (size_t)(dot - base) : strlen(base);

    snprintf(output_path, sizeof(output_path),
             "../WORKING/%.*s_r.bmp", (int)name_len, base);

    if (rotate_image(ctx->current_file, ctx->current_angle) != 0)
    {
        g_printerr("Rotation failed for %s\n", ctx->current_file);
        return;
    }

    replace_image_in_overlay(ctx->overlay, output_path);
}

GtkWidget *create_rotation_controls(GtkWidget *overlay, const char *current_file)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(box, GTK_ALIGN_END);
    gtk_widget_set_valign(box, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_widget_set_margin_end(box, 10);

    GtkWidget *plus = gtk_button_new_with_label("↻");
    GtkWidget *minus = gtk_button_new_with_label("↺");

    gtk_box_pack_start(GTK_BOX(box), minus, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), plus, FALSE, FALSE, 5);

    RotationContext *ctx = g_new0(RotationContext, 1);
    ctx->overlay = overlay;
    ctx->current_file = g_strdup(current_file);
    ctx->current_angle = 0.0;

    g_signal_connect(plus, "clicked", G_CALLBACK(on_rotate_button_clicked), ctx);
    g_signal_connect(minus, "clicked", G_CALLBACK(on_rotate_button_clicked), ctx);

    return box;
}

void replace_image_in_overlay(GtkWidget *overlay, const char *file)
{
    if (!GTK_IS_OVERLAY(overlay) || !file)
        return;

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(file, &error);
    if (!pixbuf)
    {
        g_printerr("Error loading image '%s': %s\n", file, error->message);
        g_clear_error(&error);
        return;
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(overlay));
    for (GList *iter = children; iter != NULL; iter = iter->next)
    {
        GtkWidget *child = GTK_WIDGET(iter->data);
        if (GTK_IS_IMAGE(child))
            gtk_widget_destroy(child);
    }
    g_list_free(children);

    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(overlay), image);

    g_object_unref(pixbuf);
    gtk_widget_show_all(overlay);
}

GtkWidget *create_process_button(void)
{
    GtkWidget *button = gtk_button_new_with_label("PROCESS");

    gtk_widget_set_halign(button, GTK_ALIGN_END);
    gtk_widget_set_valign(button, GTK_ALIGN_START);
    gtk_widget_set_margin_top(button, 5);
    gtk_widget_set_margin_end(button, 5);

    return button;
}
