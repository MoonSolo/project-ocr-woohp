#include <gtk/gtk.h>
#include "image_ops.h"

static void on_open_button_clicked(GtkWidget *button, gpointer user_data);
static void on_clear_button_clicked(GtkWidget *button, gpointer user_data);
static void activate(GtkApplication *app, gpointer user_data);
static void on_process_button_clicked(GtkWidget *button, gpointer user_data);


typedef struct
{
	char *current_image;
	double zoom_factor;
	GtkWidget *image_widget;
} AppContext;

static char *get_image_basename_no_ext(const char *path)
{
	char *base = g_path_get_basename(path);
	char *dot = strrchr(base, '.');
	if (dot)
		*dot = '\0';
	return base;
}

static void on_zoom_button_clicked(GtkWidget *button, gpointer user_data)
{
    AppContext *ctx = user_data;
    if (!ctx || !ctx->current_image || !ctx->image_widget) return;

    const char *label = gtk_button_get_label(GTK_BUTTON(button));
    if (!label) return;

    if (strcmp(label, "+") == 0)
        ctx->zoom_factor *= 1.2;  // zoom in
    else if (strcmp(label, "-") == 0)
        ctx->zoom_factor /= 1.2;  // zoom out

    /* Reload original image and scale */
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(ctx->current_image, &error);
    if (!pixbuf)
    {
        g_printerr("Failed to load image for zoom: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    int w = gdk_pixbuf_get_width(pixbuf) * ctx->zoom_factor;
    int h = gdk_pixbuf_get_height(pixbuf) * ctx->zoom_factor;

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, w, h, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);

    gtk_image_set_from_pixbuf(GTK_IMAGE(ctx->image_widget), scaled);
    g_object_unref(scaled);
}

static void setup_initial_window(GtkWidget *window, int w, int h)
{
    // Clear before creating (in case something was left over)
    clear_window(window);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *open_button = create_open_file_button();
    gtk_box_pack_start(GTK_BOX(box), open_button, TRUE, TRUE, 5);
    resize_button_center(open_button, w / 3, h / 10);

    gtk_widget_show_all(window);

    g_signal_connect(open_button,
                     "clicked",
                     G_CALLBACK(on_open_button_clicked),
                     NULL
    );
}

// button used once an image is loaded only
static void on_clear_button_clicked(GtkWidget *button, gpointer user_data)
{
    (void)button; // remove warning
	GtkWidget *window = GTK_WIDGET(user_data);
	AppContext *ctx = g_object_get_data(G_OBJECT(window), "ctx");
	if (ctx)
	{
		g_free(ctx->current_image);
		g_free(ctx);
		g_object_set_data(G_OBJECT(window), "ctx", NULL);
	}

    clear_window(window);
    gtk_window_resize(GTK_WINDOW(window), 800, 600);
    setup_initial_window(window, 800, 600);
}

// static meaning its private. no need for a header.
static void on_open_button_clicked(GtkWidget *button, gpointer user_data)
{
    (void)user_data;
    char *file = open_file_chooser(button);
    if (!file) return;

	AppContext *ctx = g_new0(AppContext, 1);
	ctx->current_image = g_strdup(file);

    GtkWidget *window = gtk_widget_get_toplevel(button);
    clear_window(window);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

	GtkWidget *process_button = create_process_button();
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), process_button);

	g_signal_connect(process_button,
                 "clicked",
                 G_CALLBACK(on_process_button_clicked),
                 ctx);

    GdkPixbuf *pixbuf;
    GError *error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(file, &error);
    if (!pixbuf)
    {
        g_printerr("Error loading image '%s': %s\n", file, error->message);
        g_clear_error(&error);
        g_free(file);
        return;
    }

    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(overlay), image);

	ctx->image_widget = image;
	ctx->zoom_factor = 1.0;

	GtkWidget *zoom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), zoom_box);
	gtk_widget_set_halign(zoom_box, GTK_ALIGN_START);
	gtk_widget_set_valign(zoom_box, GTK_ALIGN_END);

	GtkWidget *zoom_in_btn = gtk_button_new_with_label("+");
	gtk_box_pack_start(GTK_BOX(zoom_box), zoom_in_btn, FALSE, FALSE, 2);

	GtkWidget *zoom_out_btn = gtk_button_new_with_label("-");
	gtk_box_pack_start(GTK_BOX(zoom_box), zoom_out_btn, FALSE, FALSE, 2);

	g_signal_connect(zoom_in_btn, "clicked", G_CALLBACK(on_zoom_button_clicked), ctx);
	g_signal_connect(zoom_out_btn, "clicked", G_CALLBACK(on_zoom_button_clicked), ctx);

    GtkWidget *clear_button = create_clear_button();
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), clear_button);

    GtkWidget *rotation_controls = create_rotation_controls(overlay, file);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), rotation_controls);

    gtk_widget_show_all(window);

    g_signal_connect(clear_button,
                     "clicked",
                     G_CALLBACK(on_clear_button_clicked),
                     window);
}

// Activate opens the window
static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    GtkWidget *window;

    int WINDOW_W = 800;
    int WINDOW_H = 600;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "WOOHP - OCR");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    setup_initial_window(window, WINDOW_W, WINDOW_H);
}

static void on_process_button_clicked(GtkWidget *button, gpointer user_data)
{
    (void)button;
    AppContext *ctx = user_data;
    if (!ctx || !ctx->current_image || !ctx->image_widget) return;

    GError *error = NULL;
    char *image_name = get_image_basename_no_ext(ctx->current_image);

    char *grid_folder = g_strdup_printf("DEBUG/%s/4_grid_cells/ALL_CELLS", image_name);
    char *word_folder = g_strdup_printf("DEBUG/%s/5_letters", image_name);
    char *grid_txt = g_strdup_printf("DEBUG/%s/grid.txt", image_name);
    char *word_txt = g_strdup_printf("DEBUG/%s/words.txt", image_name);

	/* Run Image_Handler debugger */
    gchar *debugger_argv[] = {
        "../Image_Handler/debugger",
        ctx->current_image,
        NULL
    };

    if (!g_spawn_sync(NULL, debugger_argv, NULL, 0, NULL, NULL,
                      NULL, NULL, NULL, &error))
    {
        g_printerr("Debugger failed: %s\n", error->message);
        goto cleanup;
    }

    /* Run NN on grid letters */
    gchar *nn_grid_argv[] = {
        "../Neural_network/result",
        grid_folder,
        grid_txt,
        NULL
    };

    if (!g_spawn_sync(NULL, nn_grid_argv, NULL, 0, NULL, NULL,
                      NULL, NULL, NULL, &error))
    {
        g_printerr("NN grid failed: %s\n", error->message);
        goto cleanup;
    }

    /* Run NN on word letters */
    gchar *nn_word_argv[] = {
        "../Neural_network/result",
        word_folder,
        word_txt,
        NULL
    };

    if (!g_spawn_sync(NULL, nn_word_argv, NULL, 0, NULL, NULL,
                      NULL, NULL, NULL, &error))
    {
        g_printerr("NN words failed: %s\n", error->message);
        goto cleanup;
    }

    g_print("OCR done for %s\n", image_name);
    g_print("Grid → %s\n", grid_txt);
    g_print("Words → %s\n", word_txt);

	gchar *solver_argv[] = {
		"../../solver",
		grid_txt,
		word_txt,
		ctx->current_image,
		NULL
	};

	if (!g_spawn_sync(NULL, solver_argv, NULL, 0, NULL, NULL,
				NULL, NULL, NULL, &error))
	{
		g_printerr("Solver failed: %s\n", error->message);
		goto cleanup;
	}

	g_print("Solver finished successfully\n");

	/* Build solved image path */
	char *dot = strrchr(ctx->current_image, '.');
	char solved_image[1024];
	if (dot)
		snprintf(solved_image, sizeof(solved_image), "%.*s_solved%s", (int)(dot - ctx->current_image), ctx->current_image, dot);
	else
		snprintf(solved_image, sizeof(solved_image), "%s_solved.png", ctx->current_image);

	/* Update existing GtkImage pixbuf instead of recreating widgets */
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(solved_image, &error);
	if (!pixbuf)
	{
		g_printerr("Failed to load solved image: %s\n", error->message);
		goto cleanup;
	}

	/* Apply current zoom factor */
	int w = gdk_pixbuf_get_width(pixbuf) * ctx->zoom_factor;
	int h = gdk_pixbuf_get_height(pixbuf) * ctx->zoom_factor;

	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, w, h, GDK_INTERP_BILINEAR);
	g_object_unref(pixbuf);

	gtk_image_set_from_pixbuf(GTK_IMAGE(ctx->image_widget), scaled);
	g_object_unref(scaled);

	g_print("Solver finished successfully, image updated.\n");

cleanup:
	g_clear_error(&error);
	g_free(image_name);
	g_free(grid_folder);
	g_free(word_folder);
	g_free(grid_txt);
	g_free(word_txt);
}


///////////////////////////// -- REMOVE LATER -- /////////////////////////////

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("fr.epita.ocrwhoop", 0);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
