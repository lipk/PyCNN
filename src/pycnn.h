/*
   Copyright (C) 2015 by Boldizsár Lipka <lipkab@zoho.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

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
void py_set_template_custom(double (*tem)(size_t, size_t, matrix, matrix, matrix, double, void*), size_t s_val);
void py_set_boundary(double b);
void py_set_init(matrix m);
void py_set_input1(matrix m);
void py_set_input2(matrix m);
matrix py_apply_template(double dt, double t_end, int animate);

#endif
