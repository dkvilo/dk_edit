#pragma once

#include "webgpu/webgpu.h"
#include "wgpu/wgpu.h"

#define MSAA_NUMBER_OF_SAMPLE 4

WGPUShaderModule
createShaderModule(WGPUDevice device, const char* code);