#
#   Copyright (C) 2015 by Boldizsár Lipka <lipkab@zoho.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY.
#
#   See the COPYING file for more details.
#

'''
Tools for simulating cellular neural networks.

Exports:
  - Matrix is an n-by-m real matrix.
  - Template is a, uh, CNN template.
  - load_image loads an image file into a Matrix.
  - run runs the CNN simulator with the given input and template.
'''

from ctypes import *
import os
import atexit

CNN = cdll.LoadLibrary(os.curdir + "/libcnn.so.1")
CNN.init_cnn()
atexit.register(CNN.quit_cnn)

CNN.count_blacks.restype = c_size_t
CNN.count_blacks_left.restype = c_size_t
CNN.count_blacks_right.restype = c_size_t
CNN.count_blacks_top.restype = c_size_t
CNN.count_blacks_bottom.restype = c_size_t
CNN.py_load_image.restype = c_void_p

class _MatrixRaw (Structure):
    '''
    An n-by-m real matrix.

    This class allocates heap data that survives the object and will cause memory leak if not deallocated by other means. Consider using Matrix instead if that's not what you want.

    Members:
      - w is the number of columns.
      - h is the number of rows.
    
    Methods:
      - __init__ creates a new matrix with a given width and height.
      - get returns a specified item.
      - set changes a specified item.
      - expand expands the matrix by a given amount.
      - shrink shrinks the matrix by a given amount.
      - blacks counts black pixels (ones) in the matrix.
      - to_list converts the matrix into a Python list.
    '''
    _fields_ = [("w", c_size_t),
                ("h", c_size_t),
                ("data", POINTER(c_double))]

    def get(self, x, y):
        '''Return the value of the item positioned at the x-th column's y-th row.'''
        if x < 0 or x >= self.w or y < 0 or y >= self.h:
            raise ValueError("coordinate out of bounds")
        return self.data[y*self.w + x]
    
    def set(self, x, y, val):
        '''Set the value of the item positioned at the x-th column's y-th row.'''
        if x < 0 or x >= self.w or y < 0 or y >= self.h:
            raise ValueError("coordinate out of bounds")
        self.data[y*self.w + x] = c_double(val)

    def expand(self, s):
        '''Expand the matrix by s rows/columns in all directions and return the new matrix.'''
        return CNN.expand_matrix(self, c_size_t(s))
    
    def shrink(self, s):
        '''Shrink the matrix by s rows/columns in all directions and return the new matrix.'''
        if s > self.w/2:
            raise ValueError("can't shrink matrix to negative size")
        return CNN.shrink_matrix(self, c_size_t(s))
    
    def blacks(self, where = "all"):
        '''
        Count black pixels in the matrix.

        Only consider the inner items of the matrix, that is, those who have 8
        neighbors. A pixel is considered black if it's value is equal to
        or greater than 1. If where is assigned "left", "right", "top" or
        "bottom", count black pixels along the respective edges only.
        '''
        if where == "all":
            return CNN.count_blacks(self, 0)
        elif where == "left":
            return CNN.count_blacks_left(self, 0)
        elif where == "right":
            return CNN.count_blacks_right(self, 0)
        elif where == "top":
            return CNN.count_blacks_top(self, 0)
        elif where == "bottom":
            return CNN.count_blacks_bottom(self, 0)
        else:
            raise ValueError("'where' must be one of 'all', 'left', 'right', 'top' or 'bottom'");
    
    def to_list(self):
        '''Copy the items of the the matrix into a Python list of lists.'''
        res = []
        for x in range(0, self.w):
            res.append([])
            for y in range(0, self.h):
                res[-1].append(self.get(x, y))
        return res    


class Matrix (_MatrixRaw):
    '''
    An n-by-m real matrix.

    Members:
      - w is the number of columns.
      - h is the number of rows.
    
    Methods:
      - __init__ creates a new matrix with a given width and height.
      - get returns a specified item.
      - set changes a specified item.
      - expand expands the matrix by a given amount.
      - shrink shrinks the matrix by a given amount.
      - blacks counts black pixels (ones) in the matrix.
      - to_list converts the matrix into a Python list.
    '''
    
    def __new__(cls, w = 0, h = 0):
        if w < 0 or h < 0:
            raise ValueError("dimensions must be positive integers")
        return CNN.create_matrix(c_size_t(w), c_size_t(h))
    
    def __init__(self, w = 0, h = 0):
        '''Initialize a new matrix with w rows and h columns.'''
        self.w = w
        self.h = h
    
    def __del__(self):
        CNN.free_matrix(self)
    


CNN.create_matrix.restype = Matrix
CNN.expand_matrix.restype = Matrix
CNN.shrink_matrix.restype = Matrix
CNN.py_load_image.restype = Matrix
CNN.py_apply_template.restype = Matrix

class _TemplateRaw (Structure):
    _fields_ = [("a", c_double * 9),
                ("b", c_double * 9),
                ("z", c_double),
                ("d", c_double * 9),
                ("dij", c_int),
                ("dkl", c_int),
                ("phi", c_void_p),
                ("phi_data", c_void_p)]

class Template:
    '''
    Encapsulates a place and time invariant 3-by-3 CNN template.
    
    Methods:
      - __init__: initialize a new template with given coefficients.
    '''

    def _init_array_(param):
        if len(param) == 9:
            field = param
        elif len(param) == 1:
            field = [0]*9
            field[4] = param[0]
        elif len(param) == 2:
            field = [param[1]]*9
            field[4] = param[0]
        elif len(param) == 3:
            m = param[0]
            e = param[1]
            c = param[2]
            field = [c, e, c, e, m, e, c, e, c]
        else:
            raise ValueError("list must be precisely 9, 3, 2 or 1 long")
        res = (c_double * 9)()
        for i in range(0,9):
            res[i] = field[i]
        return res

    def __init__(self, a = [0]*9, b = [0]*9, z = 0, bound = 0, dt = 0.1, t_end = 10.0, d = [0]*9, dfunc = "std", dtype = "u1-x"):
        '''
        Initialize a new template with given settings.

        a and b are the state and input coefficients and can be specified in
        multiple formats. The following tables show how individual values are 
        assigned to cells when a and b are specified as a 9, 3, 2, or 1-long list
        (the numbers indiciate the position of the value in the list):

        9-long:
        1 2 3
        4 5 6
        7 8 9

        3-long:
        3 2 3
        2 1 2
        3 2 3

        2-long:
        2 2 2
        2 1 2
        2 2 2

        1-long (in this case 0 refers to the numeric 0.0 value, rather than a list item):
        0 0 0
        0 1 0
        0 0 0

        z is the bias.

        bound is the boundary condition. It can either be a numeric value between
        -1 and 1, which means constant boundary condition with the specified
        constant, or one of "periodic" and "zeroflux".

        dt and t_end tell the recommended time step and time interval for this
        template. These values may be overwritten in the actual simulation.

        Nonlinear templates are supported through the d, dfunc and dtype parameters.
        d is the coefficient matrix, dfunc is the nonlinearity, specified as
        "sd" for the standard CNN nonlinearity, "sign" for the sign function, "abs"
        for the absolute value function    or a list for a piecewise linear or
        constant function. For piecewise nonlinearities, you should avoid manual
        initialization and use the pw_const or pw_lin function instead.
        '''
        ta = Template._init_array_(a)
        tb = Template._init_array_(b)
        td = Template._init_array_(d)

        nl_func = CNN.nonlin_standard
        nl_data = None
        if type(dfunc) is str:
            if dfunc == "sign":
                nl_func = CNN.nonlin_sign
            elif dfunc == "abs":
                nl_func = CNN.nonlin_absval
        elif type(dfunc) is list:
            nl_data = (c_double * len(dfunc))()
            nl_data[0] = c_double(len(dfunc))
            for i in range(1, len(dfunc)):
                nl_data[i] = c_double(dfunc[i])

            if dfunc[0] == "const":
                nl_func = CNN.nonlin_pw_constant
            elif dfunc[0] == "lin":
                nl_func == CNN.nonlin_pw_linear
        else:
            raise TypeError("dfunc must be a string or a list")
        
        ops = ["x", "y", "u1", "u2"]
        dij = ops.index(dtype.split("-")[1])
        dkl = ops.index(dtype.split("-")[0])
        

        self.tem = _TemplateRaw(ta, tb, z, td, dij, dkl, cast(nl_func, c_void_p), cast(nl_data, c_void_p))
        self.bound = bound
        self.dt = dt
        self.t_end = t_end

def pw_const(*args):
    '''
    Constructs a list describing a piecewise constant function

    args is a sequence of numbers, interpreted as a series of coordinate pairs.
    Between point A and following point B, the value of the output function is
    equal to B's y coordinate.

    Example:
    pw_const(0, -1, 1, 1) constructs the sign function
    '''
    res = ["const"]
    for arg in args:
        res.append(arg)
    return res

def pw_lin(*args):
    '''
    Constructs a list describing a piecewise linear function.

    args is a sequence of numbers, interpreted as a series of coordinate pairs.
    The output function will consist of linear sections connecting adjacent points
    in the input series.

    Example:
    pw_lin(-1, 1, 0, 0, 1, 1) constructs the absolute value function
    '''
    res = ["lin"]
    parts = list(args)
    prev_x = parts[0]
    prev_y = parts[1]
    for i in range(1, round(len(parts)/2)):
        i = i*2
        res.append(parts[i])
        res.append((parts[i+1]-prev_y)/(parts[i]-prev_x))
        res.append(parts[i+1]-parts[i]*res[-1])
        prev_x = parts[i]
        prev_y = parts[i+1]
    return res

def load_image(path):
    '''
    Load the image file at path into a Matrix object.

    Supports JPG, PNG and BMP formats.
    '''
    return CNN.py_load_image(path.encode())

def save_image(mat, path):
    '''
    Save mat as grayscale PNG into path.
    '''
    CNN.save_image(CNN.data_to_img(mat), path.encode())

def __set_template(tem, init, input1, input2):
    bound = None
    cell_func = CFUNCTYPE(c_double, c_size_t, c_size_t, _MatrixRaw, _MatrixRaw, _MatrixRaw, c_double, c_void_p)
    s = 1
    if type(tem) is tuple:
        f = cell_func(tem[0])
        bound = tem[1]
        if len(tem) >= 3:
            s = c_size_t(tem[2])
        CNN.py_set_template_custom(f, s)
    elif type(tem) is Template:
        bound = tem.bound
        CNN.py_set_template3x3(tem.tem)
    
    if type(bound) is float:
        CNN.py_set_boundary(c_double(bound))
        CNN.fill_bounds(init, c_size_t(s), c_double(bound))
        CNN.fill_bounds(input1, c_size_t(s), c_double(bound))
        CNN.fill_bounds(input2, c_size_t(s), c_double(bound))
    elif bound == "zeroflux":
        CNN.py_set_boundary(c_double(2.0))
    elif bound == "periodic":
        CNN.py_set_boundary(c_double(3.0))

def __run_single(init, input, templ, dt = None, t_end = None, anim = False, block = False, close = True):
    input1 = input
    input2 = input
    if type(input) is tuple:
        input1 = input[0]
        input2 = input[1]
    
    if input1 is None:
        input1 = init
    if input2 is None:
        input2 = init

    init = init.expand(1)
    input1 = input1.expand(1)
    input2 = input2.expand(1)

    if dt is None:
        if type(templ) is Template:
            dt = templ.dt
        else:
            dt = 0.1
    if t_end is None:
        if type(templ) is Template:
            t_end = templ.t_end
        else:
            t_end = 10.0

    __set_template(templ, init, input1, input2)

    CNN.py_set_init(init)
    CNN.py_set_input1(input1)
    CNN.py_set_input2(input2)
    anim_flags = 0
    if anim:
        anim_flags += 1
    if block:
        anim_flags += 2
    if close:
        anim_flags += 4
    return CNN.py_apply_template(c_double(dt), c_double(t_end), anim_flags).shrink(1)

def run(init, input, templ, dt = None, t_end = None, anim = False):
    '''
    Run the CNN simulator and return the output matrix.

    init is the initial state matrix. It can be either a Matrix object or a
    string. In the latter case an image from the corresponding file will be
    loaded and used.

    input is the input matrix and can be assigned the same way as init.
    Additionally, it can also be a two-tuple, specifying two input layers, or
    None, in which case init will be used as the input as well.

    templ is the template to run. It must be either a Template object, a
    two-tuple or a three-tuple. When it is a two-tuple, the first item must be
    a callable defining the cell dynamic and the second item is the boundary
    condition. When it is a three-tuple, the third argument defines the
    connection radius. The cell function must take exactly seven arguments,
    which are, in order:
      x - the x coordinate of the current cell
      y - the y coordinate of the current cell
      state - the current state of the network (a matrix)
      input1 - the first input layer of the network (a matrix)
      input2 - the second input layer of the network (a matrix)
      t - current time
      data - a void pointer, should not be used
    The function must return a float.

    dt and t_end control the time step and total time of the simulation. When
    set to None, the default values provided by the template will be used.

    anim tells whether a visual display of the animation should be shown.

    It's also possible to run a chain of templates with just one function call.
    To do this, you need to pass a list of templates as the templ argument. When
    calling the function like this, all other arguments (except for anim) may be
    lists as well, providing different parameters for subsequent simulations or
    they can be single values. When dt, t_end or input is a single value, that
    value will be used for all simulations. When init is a single value, that
    value will be used for the first simulation and all subsequent simulations
    will use the output of the previous simulation as their initial state. When
    providing init or input as a list, the list items can either be Matrix
    objects or strings, which will be handled as described above, or integer
    values. For each value N, the output matrix of the N-th simulation will be
    substituted.

    Example:
      # Call avg on image.jpg, then call edge on the resulting image, using the
      # output of the first call as input.
      cnn.run("image.jpg", [None, 1], [avg, edge], anim = True)
    '''
    tem_list = templ
    if type(templ) is not list:
        tem_list = [templ]
    init_list = init
    if type(init) is not list:
        init_list = [init] + list(range(1, len(tem_list)))
    input_list = input
    if type(input) is not list:
        input_list = [input]*len(tem_list)
    result_list = None
    block_list = [False]*(len(tem_list)-1) + [True]
    dt_list = dt
    if type(dt) is not list:
        dt_list = [dt]*len(tem_list)
    t_end_list = t_end
    if type(t_end) is not list:
        t_end_list = [t_end]*len(tem_list)

    def get_matrix(x):
        if type(x) is int:
            return result_list[x]
        elif type(x) is str:
            return load_image(x)
        else:
            return x
    
    result_list = [get_matrix(init_list[0])]
    for i in zip(init_list, input_list, tem_list, dt_list, t_end_list, block_list):
        result_list.append(__run_single(get_matrix(i[0]), get_matrix(i[1]), i[2],  i[3], i[4], anim = anim, close = i[5], block = i[5]))
    
    return result_list[-1]


AVG = Template([2, 1, 0])
EDGE = Template(b = [8, -1], z = -1)
AND = Template([1], [1], -1)
OR = Template([1], [1], 1)
NOT = Template(b = [-1])
HL3 = Template([1.5, -0.1, -0.07], [0.32, 0.1, 0.07])
THRES = Template([2])
EROS = Template([1], [1, 1], -8, 1.0, t_end = 1.0)
DILAT = Template([1], [1, 1], 8, -1.0, t_end = 1.0)
MEDIAN = Template(a = [1], d = [0, 0.5], dfunc = "sign", dtype="u1-x")
AVGRA = Template(d = [0, 0.125], dfunc = "abs", dtype="u1-u2")
CCD_DIAG = Template([1, 0, 0, 0, 2, 0, 0, 0, -1])
CCD_HOR = Template([0, 0, 0, 1, 2, -1, 0, 0, 0])
CCD_VERT = Template([0, 1, 0, 0, 2, 0, 0, -1, 0])
CCD_MASK = Template([0, 0, 0, 1, 2, -1, 0, 0, -1], [-3], -3)
CONC_CONT = Template([0, -1, 0, -1, 3.5, -1, 0, -1, 0], [4], -4)
CONN = Template([3, 0.5, 0], [3, -0.5, 0], -4.5)
GLOB_CONN = Template([9, 6], [9, -3], -4.5)
CONTOUR = Template(a = [2], d = [0, 1], dfunc = pw_const(-0.18, 0.5, 0.18, -0.5, 1, 0.5), dtype = "u1-u1")
