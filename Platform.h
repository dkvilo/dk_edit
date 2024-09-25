#pragma once

#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "webgpu/webgpu.h"
#include "wgpu/wgpu.h"

WGPUSurface
SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window);