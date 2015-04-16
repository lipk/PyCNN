#ifndef CNN_PYCNN_H
#define CNN_PYCNN_H

#include "cnn.h"

#define CONSTANT(a) a
#define ZEROFLUX 2.0
#define PERIODIC 3.0

#define ANIMATE 1
#define BLOCK 2
#define CLOSE_WINDOW 4

matrix py_load_image(const char *file);
void py_set_template3x3(template3x3 tmpl);
void py_set_template_custom(double (*tem)(size_t, size_t, matrix, matrix, double, void*), size_t s_val);
void py_set_boundary(double b);
void py_set_init(matrix m);
void py_set_input(matrix m);
matrix py_apply_template(double dt, double t_end, int animate);

#endif
