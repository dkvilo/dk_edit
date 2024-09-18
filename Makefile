# todo (David): make order here
build:
	g++ backend/*.cpp backend/2d/*.cpp *.cpp -I./external/include ./external/lib/libwgpu_native.a ./external/lib/libSDL3.a -ggdb