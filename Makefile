CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -I./external/include -include pch.h

PCH := pch.h
PCH_GCH := pch.h.gch

SRC_BACKEND := $(wildcard backend/*.cpp backend/2d/*.cpp)
SRC_MAIN := $(wildcard *.cpp)
SRC := $(SRC_BACKEND) $(SRC_MAIN)

OBJ := $(SRC:.cpp=.o)

LIBS := ./external/lib/libwgpu_native.a ./external/lib/libSDL3.a

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

