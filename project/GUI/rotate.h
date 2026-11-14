#ifndef ROTATE_H
#define ROTATE_H

#include <gdk-pixbuf/gdk-pixbuf.h>

// to use with :
// ./rotate <image> <degrees>
int rotate_image(const char *input_path, double degrees);

#endif
