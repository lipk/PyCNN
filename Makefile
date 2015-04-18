src=src/cnn.c \
	src/pycnn.c

sdl_libs=`pkg-config sdl2 SDL2_image --libs`
sdl_cflags=`pkg-config sdl2 SDL2_image --cflags`

lib:
	gcc -c -fpic $(src) $(sdl_cflags) -std=c11 -O5
	gcc -shared -Wl,-soname,libcnn.so.1 -o libcnn.so.1 *.o -lc $(sdl_libs)
	cp src/cnn.py .
