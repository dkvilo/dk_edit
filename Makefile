CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wno-unused-value -Wno-unused-result -Wno-reorder -I./external/include -include pch.h

PCH := pch.h
PCH_GCH := pch.h.gch

SRC_BACKEND := $(wildcard backend/*.cpp backend/2d/*.cpp)
SRC_MAIN := $(wildcard *.cpp)
SRC := $(SRC_BACKEND) $(SRC_MAIN)

OBJ := $(SRC:.cpp=.o)
ARCH := $(shell uname -m)

ifeq ($(ARCH), aarch64)
    LIBS := ./external/lib/linux/aarch64/libwgpu_native.a ./external/lib/linux/aarch64/libSDL3.a -ldl -lpthread
else ifeq ($(ARCH), x86_64)
    LIBS := ./external/lib/linux/x86_64/libwgpu_native.a ./external/lib/linux/x86_64/libSDL3.a -ldl -lpthread
else
    $(error Unsupported architecture: $(ARCH))
endif

EXEC := build

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJ) $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.cpp $(PCH_GCH)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PCH_GCH): $(PCH)
	$(CXX) $(CXXFLAGS) -c $(PCH)

clean:
	rm -f $(OBJ) $(EXEC) $(PCH_GCH)

