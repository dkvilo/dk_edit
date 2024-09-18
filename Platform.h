#pragma once

#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "webgpu/webgpu.h"
#include "wgpu/wgpu.h"

// note (David): this currently working only for X11.
WGPUSurface
SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window);