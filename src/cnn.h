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

#ifndef CNN_CNN_H
#define CNN_CNN_H

#include <stdlib.h>
#include <SDL.h>

typedef struct
{
	size_t w, h;
	double *data;
} matrix ;

typedef struct
{
	double a[9];
	double b[9];
	double z;
	double d[9];
	int dij, dkl;
	double (*phi)(double, void*);
	void *phi_data;
} template3x3;

extern const matrix NULLMAT;

matrix create_matrix(size_t w, size_t h);
matrix copy_matrix(matrix m);
void free_matrix (matrix m);
void fill_matrix (matrix m, double val);

matrix img_to_data(SDL_Surface *img);
SDL_Surface *data_to_img(matrix data);
matrix load_image(const char *file);
void save_image(SDL_Surface *surf, const char *file);
matrix expand_matrix(matrix m, size_t s);
matrix shrink_matrix(matrix m, size_t s);
void fill_bounds(matrix m, size_t s, double val);

size_t count_blacks(matrix m, size_t s);
size_t count_blacks_left(matrix m, size_t s);
size_t count_blacks_right(matrix m, size_t s);
size_t count_blacks_top(matrix m, size_t s);
size_t count_blacks_bottom(matrix m, size_t s);

void bound_periodic(matrix state, size_t s);
void bound_zeroflux(matrix state, size_t s);
void bound_constant(matrix state, size_t s);

double nonlin_absval(double val, void *data);
double nonlin_sign(double val, void *data);
double nonlin_standard(double val, void *data);
double nonlin_pw_constant(double val, void *data);
double nonlin_pw_linear(double val, void *data);

double linear3x3(size_t x, size_t y, matrix state, matrix input1, matrix input2,double t, void *tem);
double nonlinear3x3(size_t x, size_t y, matrix state, matrix input1, matrix input2,double t, void *tem);

void update_animate(matrix m, void *data);
void update_nothing(matrix m, void *data);

matrix run_cnn(matrix init, matrix input1_, matrix input2_, size_t s,
			   double (*cell)(size_t, size_t, matrix, matrix, matrix, double, void*),
			   void *cell_data, void (*bnd)(matrix, size_t), double dt, double t_end,
			   void (update)(matrix, void*), void *update_data);

void init_cnn();
void quit_cnn();
#endif
