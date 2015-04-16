#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>
#include "cnn.h"

const matrix NULLMAT = {0, 0, NULL};

inline
matrix create_matrix(size_t w, size_t h)
{
	matrix mat = {w, h, (double*) malloc(sizeof(double)*w*h)};
	return mat;
}

matrix copy_matrix(matrix m)
{
	matrix newmat = create_matrix(m.w, m.h);
	memcpy(newmat.data, m.data, sizeof(double)*m.w*m.h);
	return newmat;
}

inline
void free_matrix(matrix m)
{
	free(m.data);
}

inline
void fill_matrix(matrix m, double val)
{
	memset(m.data, val, sizeof(double)*m.w*m.h);
}

double phi(double x)
{
	if (x < -1)
	{
		return -1;
	}
	else if (x > 1)
	{
		return 1;
	}
	else
	{
		return x;
	}
}

SDL_Color get_pixel(SDL_Surface *surf, int x, int y) 
{
	const SDL_PixelFormat *fmt = surf->format;
	const int i = surf->pitch*y + fmt->BytesPerPixel*x;
	const Uint32 pixel = *(Uint32*) ((Uint8*) surf->pixels + i);

	SDL_Color c;
	SDL_GetRGB(pixel, fmt, &c.r, &c.g, &c.b);
	return c;
}

void set_pixel(SDL_Surface *surf, int x, int y, SDL_Color rgb)
{
	const SDL_PixelFormat *fmt = surf->format;
	const int i = surf->pitch*y + fmt->BytesPerPixel*x;
	Uint32 pixel = *(Uint32*) ((Uint8*) surf->pixels + i);
	pixel &= ~fmt->Rmask;
	pixel &= ~fmt->Gmask;
	pixel &= ~fmt->Bmask;
	pixel &= ~fmt->Amask;

	pixel |= SDL_MapRGB(fmt, rgb.r, rgb.g, rgb.b);
	*(Uint32*) ((Uint8*) surf->pixels + i) = pixel;
}

matrix img_to_data(SDL_Surface *img)
{
	matrix mat = create_matrix(img->w, img->h);

	for (int i = 0; i<img->w; ++i)
	{
		for (int j = 0; j<img->h; ++j)
		{
			SDL_Color cl = get_pixel(img, i, j);
			const double y = (0.2126*cl.r + 0.7152*cl.g + 0.0722*cl.b)/-127.5 + 1;
			mat.data[j*img->w + i] = y;
		}
	}

	return mat;
}

SDL_Surface *data_to_img(matrix data)
{
	SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, data.w,
											 data.h, 24,
											 0xff000000,
											 0x00ff0000,
											 0x0000ff00,
											 0);
	if (!surf)
	{
		fputs(SDL_GetError(), stderr);
		return NULL;
	}
	
	for (size_t i = 0; i<data.w; ++i)
	{
		for (size_t j = 0; j<data.h; ++j)
		{
			const Uint8 rgb = (data.data[j*data.w + i]-1) * -127;
			const SDL_Color c = {rgb, rgb, rgb, 0};
			set_pixel(surf, i, j, c);
		}
	}

	return surf;
}

matrix load_image(const char *file)
{
	SDL_Surface *img = IMG_Load(file);
	if (!img)
	{
		fputs(IMG_GetError(), stderr);
		return NULLMAT;
	}

	matrix data = img_to_data(img);
	SDL_FreeSurface(img);

	return data;
}

matrix expand_matrix(matrix m, size_t s)
{
	matrix res = create_matrix(m.w + 2, m.h + 2);

	for (size_t i = 1; i<res.h-1; ++i)
	{
		memcpy(res.data + i*res.w + 1, m.data + (i-1)*m.w, sizeof(double)*m.w);
	}

	return res;
}

matrix shrink_matrix(matrix m, size_t s)
{
	matrix res = create_matrix(m.w - 2, m.h - 2);

	for (size_t i = 0; i<res.h; ++i)
	{
		memcpy(res.data + i*res.w, m.data + (i+1)*m.w + 1, sizeof(double)*res.w);
	}

	return res;
}

void fill_bounds(matrix m, size_t s, double val)
{
	for (size_t i = 0; i<s; ++i)
	{
		for (size_t j = 0; j<m.w; ++j)
		{
			m.data[m.w*i + j] = val;
			m.data[m.w*(m.h-1-i) + j] = val;
		}

		for (size_t j = 0; j<m.h; ++j)
		{
			m.data[m.w*j + i] = val;
			m.data[m.w*(m.h-1-j) + i] = val;
		}
	}
}

size_t count_blacks(matrix m, size_t s)
{
	size_t blacks = 0;
	for (size_t i = s; i<m.w-s; ++i)
	{
		for (size_t j = s; j<m.h-s; ++j)
		{
			if (m.data[j*m.w + i] >= 0.99)
			{
				blacks++;
			}
		}
	}
	return blacks;
}

size_t count_blacks_left(matrix m, size_t s)
{
	size_t blacks = 0;
	for (size_t i = s; i<m.h; ++i)
	{
		if (m.data[i*m.w + s] >= 0.99)
		{
			blacks++;
		}
	}
	return blacks;
}

size_t count_blacks_right(matrix m, size_t s)
{
	size_t blacks = 0;
	for (size_t i = s; i<m.h; ++i)
	{
		if (m.data[i*m.w + m.w-s-1] >= 0.99)
		{
			blacks++;
		}
	}
	return blacks;
}

size_t count_blacks_top(matrix m, size_t s)
{
	size_t blacks = 0;
	for (size_t i = s; i<m.w-s; ++i)
	{
		if (m.data[s*m.w + i] >= 0.99)
		{
			blacks++;
		}
	}
	return blacks;
}

size_t count_blacks_bottom(matrix m, size_t s)
{
	size_t blacks = 0;
	for (size_t i = s; i<m.w-s; ++i)
	{
		if (m.data[(m.h-s-1)*m.w + i] >= 0.99)
		{
			blacks++;
		}
	}
	return blacks;
}

double static3x3(size_t x, size_t y, matrix state, matrix input, double t, void *tem)
{
	template3x3 *tmpl = (template3x3*) tem;
	return phi((state.data[state.w*(y-1) + x-1]) * tmpl->a[0]) +
		   phi((state.data[state.w*(y-1) + x  ]) * tmpl->a[1]) +
		   phi((state.data[state.w*(y-1) + x+1]) * tmpl->a[2]) +
		   phi((state.data[state.w*(y  ) + x-1]) * tmpl->a[3]) +
		   phi((state.data[state.w*(y  ) + x  ]) * tmpl->a[4]) +
		   phi((state.data[state.w*(y  ) + x+1]) * tmpl->a[5]) +
		   phi((state.data[state.w*(y+1) + x-1]) * tmpl->a[6]) +
		   phi((state.data[state.w*(y+1) + x  ]) * tmpl->a[7]) +
		   phi((state.data[state.w*(y+1) + x+1]) * tmpl->a[8]) +
		   input.data[input.w*(y-1) + x-1] * tmpl->b[0] +
		   input.data[input.w*(y-1) + x  ] * tmpl->b[1] +
		   input.data[input.w*(y-1) + x+1] * tmpl->b[2] +
		   input.data[input.w*(y  ) + x-1] * tmpl->b[3] +
		   input.data[input.w*(y  ) + x  ] * tmpl->b[4] +
		   input.data[input.w*(y  ) + x+1] * tmpl->b[5] +
		   input.data[input.w*(y+1) + x-1] * tmpl->b[6] +
		   input.data[input.w*(y+1) + x  ] * tmpl->b[7] +
		   input.data[input.w*(y+1) + x+1] * tmpl->b[8] +
		   -state.data[state.w*y + x] + tmpl->z;
}

void bound_periodic(matrix state, size_t s)
{
	for (int i = 0; i<s; ++i)
	{
		for (size_t j = 0; j<state.w; ++j)
		{
			state.data[state.w*i + j] = state.data[(state.h-2*s+i)*state.w + j];
			state.data[state.w*(state.h-i-1) + j] = state.data[state.w*(2*s-1-i) + j];
		}

		for (size_t j = 0; j<state.h; ++j)
		{
			state.data[state.w*j + i] = state.data[state.w*j + state.w-2*s+i];
			state.data[state.w*j + state.w-i-1] = state.data[state.w*j + 2*s-1-i];
		}
	}
}

void bound_zeroflux(matrix state, size_t s)
{
	for (int i = 0; i<s; ++i)
	{
		for (size_t j = 0; j<state.w; ++j)
		{
			state.data[state.w*i + j] = state.data[state.w*s + j];
			state.data[state.w*(state.h-i-1) + j] = state.data[state.w*(state.h-s-1) + j];
		}

		for (size_t j = 0; j<state.h; ++j)
		{
			state.data[state.w*j + i] = state.data[state.w*j + s];
			state.data[state.w*j + state.w-i-1] = state.data[state.w*j + state.w-s-1];
		}
	}
}

void bound_constant(matrix state, size_t s) {}

void update_animate(matrix m, void *data)
{
	SDL_Window *window = (SDL_Window*) data;
	SDL_Surface *screen = SDL_GetWindowSurface(window);
	for (size_t x = 0; x<m.w; ++x)
	{
		for (size_t y = 0; y<m.h; ++y)
		{
			int rgb = (m.data[y*m.w+x]-1)*-127.5;
			rgb = rgb > 255 ? 255 : rgb;
			rgb = rgb < 0 ? 0 : rgb;
			SDL_Color c = {rgb, rgb, rgb, 0};
			set_pixel(screen, x, y, c);
		}
	}
	SDL_UpdateWindowSurface(window);
}

void update_nothing(matrix m, void *data) {}

matrix run_cnn(matrix init, matrix input_, size_t s,
			   double (*cell)(size_t, size_t, matrix, matrix, double, void*),
			   void *cell_data, void (*bnd)(matrix, size_t), double dt, double t_end,
			   void (*update)(matrix, void*), void *update_data)
{
	matrix buf1 = copy_matrix(init),
		   buf2 = copy_matrix(init),
		   input = copy_matrix(input_);

	matrix *state = &buf1,
		   *next_state = &buf2;
	
	bnd(input, s);

	for (double t = 0; t<t_end; t += dt)
	{
		bnd(*state, s);

		for (size_t x = s; x<state->w-s; ++x)
		{
			for (size_t y = s; y<state->h-s; ++y)
			{
				double *xy = state->data + y*state->w + x;
				const double xy_val = *xy;
				const double k1 = dt*cell(x, y, *state, input, t, cell_data);
				*xy = xy_val + k1/2;
				const double k2 = dt*cell(x, y, *state, input, t+dt/2, cell_data);
				*xy = xy_val + k2/2;
				const double k3 = dt*cell(x, y, *state, input, t+dt/2, cell_data);
				*xy = xy_val + k3;
				const double k4 = dt*cell(x, y, *state, input, t+dt, cell_data);
				*xy = xy_val;

				next_state->data[y*next_state->w + x] = phi(state->data[y*state->w + x] + k1/6 + k2/3 + k3/3 + k4/6);
			}
		}
		update(*state, update_data);
		matrix *tmp = state;
		state = next_state;
		next_state = tmp;
	}
	free_matrix(*next_state);

	return *state;
}

void init_cnn()
{
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
}

void quit_cnn()
{
	IMG_Quit();
	SDL_Quit();
}
