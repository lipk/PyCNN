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
    #pragma omp parallel for collapse(2)
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

void save_image(SDL_Surface *surf, const char *file)
{
    if (IMG_SavePNG(surf, file))
    {
        fputs(IMG_GetError(), stderr);
    }
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
    #pragma omp parallel for collapse(2) reduction(+:blacks)
    for (size_t i = s; i<m.w-s; ++i)
    {
        for (size_t j = s; j<m.h-s; ++j)
        {
            if (m.data[j*m.w + i] >= 1.0)
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
        if (m.data[i*m.w + s] >= 1.0)
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
        if (m.data[i*m.w + m.w-s-1] >= 1.0)
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
        if (m.data[s*m.w + i] >= 1.0)
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
        if (m.data[(m.h-s-1)*m.w + i] >= 1.0)
        {
            blacks++;
        }
    }
    return blacks;
}

double linear3x3(size_t x, size_t y, matrix state, matrix input1, matrix input2, double t, void *tem)
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
           input1.data[input1.w*(y-1) + x-1] * tmpl->b[0] +
           input1.data[input1.w*(y-1) + x  ] * tmpl->b[1] +
           input1.data[input1.w*(y-1) + x+1] * tmpl->b[2] +
           input1.data[input1.w*(y  ) + x-1] * tmpl->b[3] +
           input1.data[input1.w*(y  ) + x  ] * tmpl->b[4] +
           input1.data[input1.w*(y  ) + x+1] * tmpl->b[5] +
           input1.data[input1.w*(y+1) + x-1] * tmpl->b[6] +
           input1.data[input1.w*(y+1) + x  ] * tmpl->b[7] +
           input1.data[input1.w*(y+1) + x+1] * tmpl->b[8] +
           -state.data[state.w*y + x] + tmpl->z;
}


double nonlinear3x3(size_t x, size_t y, matrix state, matrix input1, matrix input2, double t, void *tem)
{
    template3x3 *tmpl = (template3x3*) tem;
    double ij[4] =
    {
        state.data[state.w*y + x],
        phi(state.data[state.w*y + x]),
        input1.data[input1.w*y + x],
        input2.data[input2.w*y + x]
    };
    double kl[4][9] =
    {
        {
            state.data[state.w*(y-1) + x-1],
            state.data[state.w*(y-1) + x  ],
            state.data[state.w*(y-1) + x+1],
            state.data[state.w*(y  ) + x-1],
            state.data[state.w*(y  ) + x  ],
            state.data[state.w*(y  ) + x+1],
            state.data[state.w*(y+1) + x-1],
            state.data[state.w*(y+1) + x  ],
            state.data[state.w*(y+1) + x+1],
        },
        {
            phi(state.data[state.w*(y-1) + x-1]),
            phi(state.data[state.w*(y-1) + x  ]),
            phi(state.data[state.w*(y-1) + x+1]),
            phi(state.data[state.w*(y  ) + x-1]),
            phi(state.data[state.w*(y  ) + x  ]),
            phi(state.data[state.w*(y  ) + x+1]),
            phi(state.data[state.w*(y+1) + x-1]),
            phi(state.data[state.w*(y+1) + x  ]),
            phi(state.data[state.w*(y+1) + x+1]),
        },
        {
            input1.data[input1.w*(y-1) + x-1],
            input1.data[input1.w*(y-1) + x  ],
            input1.data[input1.w*(y-1) + x+1],
            input1.data[input1.w*(y  ) + x-1],
            input1.data[input1.w*(y  ) + x  ],
            input1.data[input1.w*(y  ) + x+1],
            input1.data[input1.w*(y+1) + x-1],
            input1.data[input1.w*(y+1) + x  ],
            input1.data[input1.w*(y+1) + x+1],
        },
        {
            input2.data[input2.w*(y-1) + x-1],
            input2.data[input2.w*(y-1) + x  ],
            input2.data[input2.w*(y-1) + x+1],
            input2.data[input2.w*(y  ) + x-1],
            input2.data[input2.w*(y  ) + x  ],
            input2.data[input2.w*(y  ) + x+1],
            input2.data[input2.w*(y+1) + x-1],
            input2.data[input2.w*(y+1) + x  ],
            input2.data[input2.w*(y+1) + x+1],
        }
    };

    return -state.data[state.w*y + x] + tmpl->z +
           + kl[1][0]*tmpl->a[0]
           + kl[1][1]*tmpl->a[1]
           + kl[1][2]*tmpl->a[2]
           + kl[1][3]*tmpl->a[3]
           + kl[1][4]*tmpl->a[4]
           + kl[1][5]*tmpl->a[5]
           + kl[1][6]*tmpl->a[6]
           + kl[1][7]*tmpl->a[7]
           + kl[1][8]*tmpl->a[8]
           + kl[2][0]*tmpl->b[0]
           + kl[2][1]*tmpl->b[1]
           + kl[2][2]*tmpl->b[2]
           + kl[2][3]*tmpl->b[3]
           + kl[2][4]*tmpl->b[4]
           + kl[2][5]*tmpl->b[5]
           + kl[2][6]*tmpl->b[6]
           + kl[2][7]*tmpl->b[7]
           + kl[2][8]*tmpl->b[8]
           + tmpl->phi(kl[tmpl->dkl][0]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[0]
           + tmpl->phi(kl[tmpl->dkl][1]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[1]
           + tmpl->phi(kl[tmpl->dkl][2]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[2]
           + tmpl->phi(kl[tmpl->dkl][3]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[3]
           + tmpl->phi(kl[tmpl->dkl][4]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[4]
           + tmpl->phi(kl[tmpl->dkl][5]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[5]
           + tmpl->phi(kl[tmpl->dkl][6]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[6]
           + tmpl->phi(kl[tmpl->dkl][7]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[7]
           + tmpl->phi(kl[tmpl->dkl][8]-ij[tmpl->dij],tmpl->phi_data)*tmpl->d[8];
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

double nonlin_absval(double val, void *data)
{
    if (val < 0)
    {
        return -val;
    }
    else
    {
        return val;
    }
}

double nonlin_sign(double val, void *data)
{
    if (val < 0)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

double nonlin_standard(double val, void *data)
{
    if (val < -1)
    {
        return -1;
    }
    else if (val > 1)
    {
        return 1;
    }
    else
    {
        return val;
    }
}

double nonlin_pw_constant(double val, void *data)
{
    double *p = (double*) data;
    for (int i = 1; i<p[0]; i += 2)
    {
        if (val < p[i])
        {
            return p[i+1];
        }
    }
    return p[(int) p[0]-1];
}

double nonlin_pw_linear(double val, void *data)
{
    double *p = (double*) data;
    for (int i = 1; i<p[0]; i += 3)
    {
        if (val < p[i])
        {
            return val*p[i+1] + p[i+2];
        }
    }
    return val*p[(int) p[0]-2] + p[(int) p[0]-1];
}

void update_animate(matrix m, void *data)
{
    SDL_Window *window = (SDL_Window*) data;
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    #pragma omp parallel for collapse(2)
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

matrix run_cnn(matrix init, matrix input1_, matrix input2_, size_t s,
               double (*cell)(size_t, size_t, matrix, matrix, matrix, double, void*),
               void *cell_data, void (*bnd)(matrix, size_t), double dt, double t_end,
               void (*update)(matrix, void*), void *update_data)
{
    matrix buf1 = copy_matrix(init),
           buf2 = copy_matrix(init),
           input1 = copy_matrix(input1_),
           input2 = copy_matrix(input2_);

    matrix *state = &buf1,
           *next_state = &buf2;
    
    bnd(input1, s);
    bnd(input2, s);

    for (double t = 0; t<t_end; t += dt)
    {
        bnd(*state, s);
        #pragma omp parallel for collapse(2)
        for (size_t x = s; x<state->w-s; ++x)
        {
            for (size_t y = s; y<state->h-s; ++y)
            {
                double *xy = state->data + y*state->w + x;
                const double xy_val = *xy;
                const double k1 = dt*cell(x, y, *state, input1, input2, t, cell_data);
                *xy = xy_val + k1/2;
                const double k2 = dt*cell(x, y, *state, input1, input2, t+dt/2, cell_data);
                *xy = xy_val + k2/2;
                const double k3 = dt*cell(x, y, *state, input1, input2, t+dt/2, cell_data);
                *xy = xy_val + k3;
                const double k4 = dt*cell(x, y, *state, input1, input2, t+dt, cell_data);
                *xy = xy_val;

                next_state->data[y*next_state->w + x] = state->data[y*state->w + x] + k1/6 + k2/3 + k3/3 + k4/6;
            }
        }
        update(*state, update_data);
        matrix *tmp = state;
        state = next_state;
        next_state = tmp;
    }
    
    #pragma omp parallel for collapse(2)
    for (size_t x = 0; x<state->w; ++x)
    {
        for (size_t y = 0; y<state->h; ++y)
        {
            state->data[y*state->w + x] = phi(state->data[y*state->w + x]);
        }
    }

    free_matrix(*next_state);
    free_matrix(input1);
    free_matrix(input2);

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
