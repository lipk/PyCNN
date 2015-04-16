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
} template3x3;

extern const matrix NULLMAT;

matrix create_matrix(size_t w, size_t h);
matrix copy_matrix(matrix m);
void free_matrix (matrix m);
void fill_matrix (matrix m, double val);

matrix img_to_data(SDL_Surface *img);
SDL_Surface *data_to_img(matrix data);
matrix load_image(const char *file);
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

double static3x3(size_t x, size_t y, matrix state, matrix input, double t, void *tem);
double staticNxN(size_t x, size_t y, matrix state, matrix input, double t, void *tem);

void update_animate(matrix m, void *data);
void update_nothing(matrix m, void *data);

matrix run_cnn(matrix init, matrix input_, size_t s,
			   double (*cell)(size_t, size_t, matrix, matrix, double, void*),
			   void *cell_data, void (*bnd)(matrix, size_t), double dt, double t_end,
			   void (update)(matrix, void*), void *update_data);

void init_cnn();
void quit_cnn();
#endif
