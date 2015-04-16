#include "pycnn.h"
#include <SDL.h>

template3x3 tem3x3;
void *tem_data;
double (*tem_func)(size_t, size_t, matrix, matrix, double, void*);
matrix init;
matrix input;
double bnd;
size_t s;

SDL_Window *window = NULL;

matrix py_load_image(const char *file)
{
	matrix mat = load_image(file);
	matrix res = expand_matrix(mat, 1);
	free_matrix(mat);
	return res;
}

void py_set_template3x3(template3x3 tm)
{
	tem3x3 = tm;
	tem_func = static3x3;
	tem_data = &tem3x3;
	s = 1;
}

void py_set_template_custom(double (*tem)(size_t, size_t, matrix, matrix, double, void*), size_t s_val)
{
	tem_func = tem;
	tem_data = NULL;
	s = s_val;
}

void py_set_boundary(double b)
{
	bnd = b;
}

void py_set_init(matrix m)
{
	init = m;
}

void py_set_input(matrix m)
{
	input = m;
}


matrix py_apply_template(double dt, double t_end, int anim)
{
	fill_bounds(init, 1, bnd);
	fill_bounds(input, 1, bnd);

	void (*upd_func)(matrix, void*) = update_nothing;
	void *upd_data = NULL;

	if (anim & ANIMATE)
	{
		upd_func = update_animate;
		if (!window)
		{
			window = SDL_CreateWindow("CNN", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
									  init.w, init.h, SDL_WINDOW_SHOWN);
		}
		upd_data = window;
	}

	void (*bnd_func)(matrix, size_t) = NULL;
	if (bnd == ZEROFLUX)
	{
		bnd_func = bound_zeroflux;
	}
	else if (bnd == PERIODIC)
	{
		bnd_func = bound_periodic;
	}
	else
	{
		bnd_func = bound_constant;
	}

	matrix res = run_cnn(init, input, 1, tem_func, tem_data, bnd_func, dt, t_end, upd_func, upd_data);
	
	if (anim & BLOCK && anim & ANIMATE)
	{
		SDL_Event ev;
		while (1)
		{
			SDL_PollEvent(&ev);
			if (ev.type == SDL_QUIT || ev.type == SDL_KEYDOWN)
			{
				break;
			}
		}
	}

	if (anim & CLOSE_WINDOW && anim & ANIMATE)
	{
		SDL_DestroyWindow(window);
		window = NULL;
	}

	return res;
}
