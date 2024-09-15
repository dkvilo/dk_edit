build:
	g++ *.cpp -I./external/include ./external/lib/libwgpu_native.a ./external/lib/libSDL3.a -ggdb