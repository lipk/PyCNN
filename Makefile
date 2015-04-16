src=cnn.c
demo_src=main.c
lib_src=pycnn.c

sdl_libs=`pkg-config sdl2 SDL2_image --libs`
sdl_cflags=`pkg-config sdl2 SDL2_image --cflags`

lib:
	gcc -c -fpic cnn.c pycnn.c $(sdl_cflags) -std=c11 -O5
	gcc -shared -Wl,-soname,libcnn.so.1 -o libcnn.so.1 *.o -lc $(sdl_libs)
