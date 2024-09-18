#include "common.h"

WGPUShaderModule
createShaderModule(WGPUDevice device, const char* code)
{
  WGPUShaderModuleWGSLDescriptor wgslDesc = {};
  wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
  wgslDesc.code = code;

  WGPUShaderModuleDescriptor shaderDesc = {};
  shaderDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
  return wgpuDeviceCreateShaderModule(device, &shaderDesc);
}