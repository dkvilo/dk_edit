#include "Renderer.h"
#include "../common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include <algorithm>
#include <iostream>
#include <math.h>

BatchRenderer::BatchRenderer(WGPUDevice device,
                             WGPUQueue queue,
                             int32_t width,
                             int32_t height)
  : device(device)
  , queue(queue)
  , windowWidth(width)
  , windowHeight(height)
  , numQuads(0)
  , vertexBuffer(nullptr)
  , indexBuffer(nullptr)
  , uniformBuffer(nullptr)
  , pipeline(nullptr)
  , vertexShaderModule(nullptr)
  , fragmentShaderModule(nullptr)
  , bindGroupLayout(nullptr)
  , bindGroup(nullptr)
{
}

BatchRenderer::~BatchRenderer()
{
  if (vertexBuffer)
    wgpuBufferRelease(vertexBuffer);
  if (indexBuffer)
    wgpuBufferRelease(indexBuffer);
  if (pipeline)
    wgpuRenderPipelineRelease(pipeline);
  if (vertexShaderModule)
    wgpuShaderModuleRelease(vertexShaderModule);
  if (fragmentShaderModule)
    wgpuShaderModuleRelease(fragmentShaderModule);
  if (bindGroupLayout)
    wgpuBindGroupLayoutRelease(bindGroupLayout);
  if (bindGroup)
    wgpuBindGroupRelease(bindGroup);
  for (int32_t i = 0; i < 2; ++i) {
    if (textures[i].texture)
      wgpuTextureRelease(textures[i].texture);
    if (textures[i].textureView)
      wgpuTextureViewRelease(textures[i].textureView);
    if (textures[i].sampler)
      wgpuSamplerRelease(textures[i].sampler);
  }
  if (fontData.fontBuffer)
    delete[] fontData.fontBuffer;
  if (fontData.bitmap)
    delete[] fontData.bitmap;
}

void
BatchRenderer::Initialize()
{
  CreatePipeline();
  CreateBuffers();
  LoadTexture("res/spritesheet.png", 0);
  LoadFont("res/JetBrainsMono-Regular.ttf"); // this will be bound to 1
  CreateBindGroup();
}

void
BatchRenderer::CreatePipeline()
{
  const char* vertexShaderCode = R"(
      struct Uniforms {
          uTime: f32,
          uProjection: mat4x4<f32>,
      };

      @group(0) @binding(4) var<uniform> uniforms: Uniforms;

      struct VertexInput {
          @location(0) position : vec2<f32>,
          @location(1) color    : vec4<f32>,
          @location(2) texCoord : vec2<f32>,
          @location(3) rotation : f32,
          @location(4) texIndex : u32,
          @location(5) translation : vec2<f32>,
      };

      struct VertexOutput {
          @builtin(position) Position : vec4<f32>,
          @location(0) color          : vec4<f32>,
          @location(1) texCoord       : vec2<f32>,
          @location(2) texIndex       : u32,
      };

      @vertex
      fn main(input : VertexInput) -> VertexOutput {
          // rotation matrix
          let cosTheta = cos(input.rotation);
          let sinTheta = sin(input.rotation);

          let rotationMatrix = mat2x2<f32>(
              cosTheta, -sinTheta,
              sinTheta, cosTheta
          );

          // rotation
          let rotatedPosition = rotationMatrix * input.position;

          // translation
          let translatedPosition = rotatedPosition + input.translation;

          // projection
          var output: VertexOutput;
          output.Position = uniforms.uProjection * vec4<f32>(translatedPosition, 0.0, 1.0);
          output.color = input.color;
          output.texCoord = input.texCoord;
          output.texIndex = input.texIndex;
          return output;
      }
    )";

  const char* fragmentShaderCode = R"(
      @group(0) @binding(0) var myTexture0: texture_2d<f32>;
      @group(0) @binding(1) var mySampler0: sampler;
      @group(0) @binding(2) var myTexture1: texture_2d<f32>;
      @group(0) @binding(3) var mySampler1: sampler;

      @fragment
      fn main(
          @location(0) color : vec4<f32>,
          @location(1) texCoord : vec2<f32>,
          @location(2) texIndex : u32
      ) -> @location(0) vec4<f32> {
          var finalColor: vec4<f32>;
          if (texIndex == 2u) {
              // Sample the font texture and use the red channel as alpha
              let sampledAlpha = textureSample(myTexture1, mySampler1, texCoord).r;
              finalColor = vec4<f32>(color.rgb, color.a * sampledAlpha);
          } else if (texIndex == 1u) {
              let texColor = textureSample(myTexture0, mySampler0, texCoord);
              finalColor = texColor * color;
          } else {
              finalColor = color;
          }
          return finalColor;
      }
    )";

  vertexShaderModule = createShaderModule(device, vertexShaderCode);
  fragmentShaderModule = createShaderModule(device, fragmentShaderCode);

  WGPUVertexAttribute attributes[6];
  attributes[0] = { WGPUVertexFormat_Float32x2, offsetof(Vertex, position), 0 };
  attributes[1] = { WGPUVertexFormat_Float32x4, offsetof(Vertex, color), 1 };
  attributes[2] = { WGPUVertexFormat_Float32x2, offsetof(Vertex, texCoord), 2 };
  attributes[3] = { WGPUVertexFormat_Float32, offsetof(Vertex, rotation), 3 };
  attributes[4] = { WGPUVertexFormat_Uint32, offsetof(Vertex, texIndex), 4 };
  attributes[5] = { WGPUVertexFormat_Float32x2,
                    offsetof(Vertex, translation),
                    5 };

  std::cout << "sizeof(Vertex): " << sizeof(Vertex) << std::endl;
  std::cout << "offsetof(position): " << offsetof(Vertex, position)
            << std::endl;
  std::cout << "offsetof(color): " << offsetof(Vertex, color) << std::endl;
  std::cout << "offsetof(texCoord): " << offsetof(Vertex, texCoord)
            << std::endl;
  std::cout << "offsetof(rotation): " << offsetof(Vertex, rotation)
            << std::endl;
  std::cout << "offsetof(texIndex): " << offsetof(Vertex, texIndex)
            << std::endl;
  std::cout << "offsetof(translation): " << offsetof(Vertex, translation)
            << std::endl;

  // vertex buffer layout
  WGPUVertexBufferLayout vertexBufferLayout = {};
  vertexBufferLayout.arrayStride = sizeof(Vertex);
  vertexBufferLayout.attributeCount = 6;
  vertexBufferLayout.attributes = attributes;
  vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

  // bind group layout
  WGPUBindGroupLayoutEntry bglEntries[5] = {};
  bglEntries[0].binding = 0;
  bglEntries[0].visibility = WGPUShaderStage_Fragment;
  bglEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
  bglEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
  bglEntries[0].texture.multisampled = false;

  bglEntries[1].binding = 1;
  bglEntries[1].visibility = WGPUShaderStage_Fragment;
  bglEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

  bglEntries[2].binding = 2;
  bglEntries[2].visibility = WGPUShaderStage_Fragment;
  bglEntries[2].texture.sampleType = WGPUTextureSampleType_Float;
  bglEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
  bglEntries[2].texture.multisampled = false;

  bglEntries[3].binding = 3;
  bglEntries[3].visibility = WGPUShaderStage_Fragment;
  bglEntries[3].sampler.type = WGPUSamplerBindingType_Filtering;

  bglEntries[4].binding = 4;
  bglEntries[4].visibility = WGPUShaderStage_Vertex;
  bglEntries[4].buffer.type = WGPUBufferBindingType_Uniform;
  bglEntries[4].buffer.minBindingSize = sizeof(Uniforms);

  WGPUBindGroupLayoutDescriptor bglDesc = {};
  bglDesc.entryCount = 5;
  bglDesc.entries = bglEntries;

  bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

  // pipeline layout
  WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
  pipelineLayoutDesc.bindGroupLayoutCount = 1;
  pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
  WGPUPipelineLayout pipelineLayout =
    wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

  // fragment state
  WGPUBlendState blendState = {};
  blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
  blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blendState.color.operation = WGPUBlendOperation_Add;
  blendState.alpha.srcFactor = WGPUBlendFactor_One;
  blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
  blendState.alpha.operation = WGPUBlendOperation_Add;

  WGPUColorTargetState colorTargetState = {};
  colorTargetState.format = WGPUTextureFormat_BGRA8Unorm;
  colorTargetState.blend = &blendState;
  colorTargetState.writeMask = WGPUColorWriteMask_All;

  WGPUFragmentState fragmentState = {};
  fragmentState.module = fragmentShaderModule;
  fragmentState.entryPoint = "main";
  fragmentState.constantCount = 0;
  fragmentState.constants = nullptr;
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTargetState;

  // render pipeline
  WGPURenderPipelineDescriptor pipelineDesc = {};
  pipelineDesc.layout = pipelineLayout;
  pipelineDesc.vertex = {};
  pipelineDesc.vertex.module = vertexShaderModule;
  pipelineDesc.vertex.entryPoint = "main";
  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;
  pipelineDesc.fragment = &fragmentState;
  pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
  pipelineDesc.primitive.cullMode = WGPUCullMode_None;

  pipelineDesc.multisample.count = MSAA_NUMBER_OF_SAMPLE;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = false;

  pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

  wgpuPipelineLayoutRelease(pipelineLayout);
}

void
BatchRenderer::CreateBuffers()
{
  const size_t maxVertices = MAX_QUADS * 4;
  const size_t maxIndices = MAX_QUADS * 6;

  // vbo
  WGPUBufferDescriptor vertexBufferDesc = {};
  vertexBufferDesc.size = sizeof(Vertex) * maxVertices;
  vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
  vertexBufferDesc.mappedAtCreation = false;
  vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);

  // ibo
  WGPUBufferDescriptor indexBufferDesc = {};
  indexBufferDesc.size = sizeof(uint16_t) * maxIndices;
  indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
  indexBufferDesc.mappedAtCreation = false;
  indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);

  // Uniform buffer
  WGPUBufferDescriptor uniformBufferDesc = {};
  uniformBufferDesc.size = sizeof(Uniforms);
  uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  uniformBufferDesc.mappedAtCreation = false;
  uniformBuffer = wgpuDeviceCreateBuffer(device, &uniformBufferDesc);
  if (!uniformBuffer) {
    std::cerr << "Failed to create uniform buffer!" << std::endl;
    return;
  }

  // initial uniform values
  Uniforms initialUniforms = {};
  initialUniforms.uProjection = matrix4_identity();

  wgpuQueueWriteBuffer(
    queue, uniformBuffer, 0, &initialUniforms, sizeof(Uniforms));
}

void
BatchRenderer::LoadTexture(const char* filePath, int32_t textureIndex)
{
  int32_t texWidth, texHeight, texChannels;
  stbi_uc* pixels =
    stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  if (!pixels) {
    std::cout << "Failed to load texture: " << filePath << std::endl;
    return;
  }

  textures[textureIndex].width = texWidth;
  textures[textureIndex].height = texHeight;

  // texture
  WGPUTextureDescriptor textureDesc = {};
  textureDesc.size.width = texWidth;
  textureDesc.size.height = texHeight;
  textureDesc.size.depthOrArrayLayers = 1;
  textureDesc.mipLevelCount = 1;
  textureDesc.sampleCount = 1;
  textureDesc.dimension = WGPUTextureDimension_2D;
  textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
  textureDesc.usage =
    WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

  textures[textureIndex].texture =
    wgpuDeviceCreateTexture(device, &textureDesc);

  // load data to texture
  WGPUImageCopyTexture copyTexture = {
    .texture = textures[textureIndex].texture,
    .mipLevel = 0,
    .origin = { 0, 0, 0 },
    .aspect = WGPUTextureAspect_All,
  };

  WGPUTextureDataLayout textureDataLayout = {
    .offset = 0,
    .bytesPerRow = (uint32_t)(texWidth * 4),
    .rowsPerImage = (uint32_t)texHeight,
  };

  WGPUExtent3D textureExtent3D{
    .width = (uint32_t)texWidth,
    .height = (uint32_t)texHeight,
    .depthOrArrayLayers = 1,
  };
  wgpuQueueWriteTexture(queue,
                        &copyTexture,
                        pixels,
                        texWidth * texHeight * 4,
                        &textureDataLayout,
                        &textureExtent3D);

  stbi_image_free(pixels);

  // texture view
  WGPUTextureViewDescriptor viewDesc = {};
  viewDesc.format = WGPUTextureFormat_RGBA8Unorm;
  viewDesc.dimension = WGPUTextureViewDimension_2D;
  viewDesc.baseMipLevel = 0;
  viewDesc.mipLevelCount = 1;
  viewDesc.baseArrayLayer = 0;
  viewDesc.arrayLayerCount = 1;
  viewDesc.aspect = WGPUTextureAspect_All;

  textures[textureIndex].textureView =
    wgpuTextureCreateView(textures[textureIndex].texture, &viewDesc);

  // sampler
  WGPUSamplerDescriptor samplerDesc = {};
  samplerDesc.addressModeU = WGPUAddressMode_Repeat;
  samplerDesc.addressModeV = WGPUAddressMode_Repeat;
  samplerDesc.addressModeW = WGPUAddressMode_Repeat;
  samplerDesc.magFilter = WGPUFilterMode_Nearest;
  samplerDesc.minFilter = WGPUFilterMode_Nearest;
  samplerDesc.lodMinClamp = 0.0f;
  samplerDesc.lodMaxClamp = 32.0f;
  samplerDesc.maxAnisotropy = 1;

  textures[textureIndex].sampler =
    wgpuDeviceCreateSampler(device, &samplerDesc);
}

Vector4
RGBA32(uint32_t hexColor)
{
  float red = ((hexColor >> 16) & 0xFF) / 255.0f;
  float green = ((hexColor >> 8) & 0xFF) / 255.0f;
  float blue = (hexColor & 0xFF) / 255.0f;
  float alpha = 1.0f;
  return { red, green, blue, alpha };
}

void
BatchRenderer::LoadFont(const char* fontFilePath)
{
  FILE* fontFile = fopen(fontFilePath, "rb");
  if (!fontFile) {
    std::cout << "Failed to load font: " << fontFilePath << std::endl;
    return;
  }

  fseek(fontFile, 0, SEEK_END);
  long fontSize = ftell(fontFile);
  fseek(fontFile, 0, SEEK_SET);

  fontData.fontBuffer = new unsigned char[fontSize];
  (void*)fread(fontData.fontBuffer, 1, fontSize, fontFile);
  fclose(fontFile);

  stbtt_InitFont(&fontData.fontInfo,
                 fontData.fontBuffer,
                 stbtt_GetFontOffsetForIndex(fontData.fontBuffer, 0));

  // bitmap atlas
  const int32_t bitmapWidth = 512;
  const int32_t bitmapHeight = 512;
  fontData.bitmap = new unsigned char[bitmapWidth * bitmapHeight];

  // bake tha font bitmap braw
  float pixelHeight = 90.0f;
  int32_t result = stbtt_BakeFontBitmap(fontData.fontBuffer,
                                        0,
                                        pixelHeight,
                                        fontData.bitmap,
                                        bitmapWidth,
                                        bitmapHeight,
                                        32,
                                        96,
                                        fontData.cdata);
  if (result <= 0) {
    std::cout << "Failed to bake font bitmap." << std::endl;
    return;
  }

  WGPUTextureDescriptor textureDesc = {};
  textureDesc.size.width = bitmapWidth;
  textureDesc.size.height = bitmapHeight;
  textureDesc.size.depthOrArrayLayers = 1;
  textureDesc.mipLevelCount = 1;
  textureDesc.sampleCount = 1;
  textureDesc.dimension = WGPUTextureDimension_2D;
  textureDesc.format = WGPUTextureFormat_R8Unorm; // single channel
  textureDesc.usage =
    WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

  WGPUTexture fontTexture = wgpuDeviceCreateTexture(device, &textureDesc);
  WGPUImageCopyTexture copyTexture = {
    .texture = fontTexture,
    .mipLevel = 0,
    .origin = { 0, 0, 0 },
    .aspect = WGPUTextureAspect_All,
  };

  WGPUTextureDataLayout textureDataLayout = {
    .offset = 0,
    .bytesPerRow = bitmapWidth * 1, // R8Unorm = 1 byte per pixel
    .rowsPerImage = bitmapHeight,
  };

  WGPUExtent3D textureExtent3D{
    .width = bitmapWidth,
    .height = bitmapHeight,
    .depthOrArrayLayers = 1,
  };

  wgpuQueueWriteTexture(queue,
                        &copyTexture,
                        fontData.bitmap,
                        bitmapWidth * bitmapHeight * 1, // total size in bytes
                        &textureDataLayout,
                        &textureExtent3D);

  // texture view
  WGPUTextureViewDescriptor viewDesc = {};
  viewDesc.format = WGPUTextureFormat_R8Unorm;
  viewDesc.dimension = WGPUTextureViewDimension_2D;
  viewDesc.baseMipLevel = 0;
  viewDesc.mipLevelCount = 1;
  viewDesc.baseArrayLayer = 0;
  viewDesc.arrayLayerCount = 1;
  viewDesc.aspect = WGPUTextureAspect_All;

  WGPUTextureView fontTextureView =
    wgpuTextureCreateView(fontTexture, &viewDesc);

  WGPUSamplerDescriptor samplerDesc = {};
  samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
  samplerDesc.magFilter = WGPUFilterMode_Linear;
  samplerDesc.minFilter = WGPUFilterMode_Linear;
  samplerDesc.lodMinClamp = 0.0f;
  samplerDesc.lodMaxClamp = 32.0f;
  samplerDesc.maxAnisotropy = 1;

  WGPUSampler fontSampler = wgpuDeviceCreateSampler(device, &samplerDesc);

  textures[1].texture = fontTexture;
  textures[1].textureView = fontTextureView;
  textures[1].sampler = fontSampler;
  textures[1].width = bitmapWidth;
  textures[1].height = bitmapHeight;
}

void
BatchRenderer::CreateBindGroup()
{
  WGPUBindGroupEntry bgEntries[5] = {};

  // Texture 0
  bgEntries[0].binding = 0;
  bgEntries[0].textureView = textures[0].textureView;
  bgEntries[1].binding = 1;
  bgEntries[1].sampler = textures[0].sampler;

  // Texture 1 (Font)
  bgEntries[2].binding = 2;
  bgEntries[2].textureView = textures[1].textureView;
  bgEntries[3].binding = 3;
  bgEntries[3].sampler = textures[1].sampler;

  // Uniform buffer
  bgEntries[4].binding = 4;
  bgEntries[4].buffer = uniformBuffer;
  bgEntries[4].offset = 0;
  bgEntries[4].size = sizeof(Uniforms);

  // Validate bind group entries
  for (int i = 0; i < 4; i++) {
    if (!bgEntries[i].textureView && !bgEntries[i].sampler) {
      std::cerr << "Invalid texture/sampler at binding " << i << std::endl;
      return;
    }
  }
  if (!bgEntries[4].buffer) {
    std::cerr << "Uniform buffer not properly set at binding 4!" << std::endl;
    return;
  }

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.layout = bindGroupLayout;
  bgDesc.entryCount = 5;
  bgDesc.entries = bgEntries;

  bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

  // Check bind group creation
  if (!bindGroup) {
    std::cerr << "Failed to create bind group!" << std::endl;
  } else {
    std::cout << "Bind group successfully created!" << std::endl;
  }
}

void
BatchRenderer::AddQuad(Vector2 position,
                       float width,
                       float height,
                       Vector4 color,
                       float rotation,
                       Vector2 origin,
                       int32_t drawOrder = 0)
{
  if (quads.size() >= MAX_QUADS)
    return;

  Quad quad;
  quad.drawOrder = drawOrder;

  // origin offset
  float originX = origin.x * width;
  float originY = origin.y * height;

  Vertex v0 = { { -originX, height - originY }, // Top-left
                color,
                { 0.0f, 1.0f },
                rotation,
                0u,
                position,
                0.0f };

  Vertex v1 = { { width - originX, height - originY }, // Top-right
                color,
                { 1.0f, 1.0f },
                rotation,
                0u,
                position,
                0.0f };

  Vertex v2 = { { width - originX, -originY }, // Bottom-right
                color,
                { 1.0f, 0.0f },
                rotation,
                0u,
                position,
                0.0f };

  Vertex v3 = { { -originX, -originY }, // Bottom-left
                color,
                { 0.0f, 0.0f },
                rotation,
                0u,
                position,
                0.0f };

  quad.vertices = { v0, v1, v2, v3 };
  quad.indices = { 0, 1, 2, 0, 2, 3 };

  quads.push_back(quad);
}

void
BatchRenderer::AddLine(Vector2 start,
                       Vector2 end,
                       float thickness,
                       Vector4 color,
                       float rotation = 0.0f,
                       Vector2 origin = ORIGIN_CENTER,
                       int32_t drawOrder = 0)
{
  if (quads.size() >= MAX_QUADS)
    return;

  Quad quad;
  quad.drawOrder = drawOrder;

  Vector2 dir = { end.x - start.x, end.y - start.y };
  float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

  if (length == 0.0f)
    return; // cannot create a line with zero length

  Vector2 normDir = { dir.x / length, dir.y / length };
  Vector2 perp = { -normDir.y, normDir.x };

  float halfThickness = thickness / 2.0f;
  Vector2 offset = { perp.x * halfThickness, perp.y * halfThickness };

  // center point of the line
  Vector2 center = { (start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f };

  // vertices relative to the center
  Vector2 v1 = { start.x - center.x + offset.x, start.y - center.y + offset.y };
  Vector2 v2 = { start.x - center.x - offset.x, start.y - center.y - offset.y };
  Vector2 v3 = { end.x - center.x - offset.x, end.y - center.y - offset.y };
  Vector2 v4 = { end.x - center.x + offset.x, end.y - center.y + offset.y };

  Vertex vertex0 = { { v1.x, v1.y }, color, { 0.0f, 0.0f }, rotation, 0u,
                     center,         0.0f };
  Vertex vertex1 = { { v2.x, v2.y }, color, { 0.0f, 0.0f }, rotation, 0u,
                     center,         0.0f };
  Vertex vertex2 = { { v3.x, v3.y }, color, { 0.0f, 0.0f }, rotation, 0u,
                     center,         0.0f };
  Vertex vertex3 = { { v4.x, v4.y }, color, { 0.0f, 0.0f }, rotation, 0u,
                     center,         0.0f };

  quad.vertices = { vertex0, vertex1, vertex2, vertex3 };
  quad.indices = { 0, 1, 2, 0, 2, 3 };

  quads.push_back(quad);
}

void
BatchRenderer::AddTexturedQuad(Vector2 position,
                               float width,
                               float height,
                               float texCoords[4][2],
                               uint32_t texIndex,
                               Vector4 color,
                               float rotation = 0.0f,
                               Vector2 origin = ORIGIN_CENTER,
                               int32_t drawOrder = 0)
{
  if (quads.size() >= MAX_QUADS)
    return;

  Quad quad;
  quad.drawOrder = drawOrder;

  // origin offset
  float originX = origin.x * width;
  float originY = origin.y * height;

  Vertex v0 = { { -originX, height - originY }, // Top-left
                color,
                { texCoords[0][0], texCoords[0][1] },
                rotation,
                texIndex,
                position,
                0.0f };

  Vertex v1 = { { width - originX, height - originY }, // Top-right
                color,
                { texCoords[1][0], texCoords[1][1] },
                rotation,
                texIndex,
                position,
                0.0f };

  Vertex v2 = { { width - originX, -originY }, // Bottom-right
                color,
                { texCoords[2][0], texCoords[2][1] },
                rotation,
                texIndex,
                position,
                0.0f };

  Vertex v3 = { { -originX, -originY }, // Bottom-left
                color,
                { texCoords[3][0], texCoords[3][1] },
                rotation,
                texIndex,
                position,
                0.0f };

  quad.vertices = { v0, v1, v2, v3 };
  quad.indices = { 0, 1, 2, 0, 2, 3 };

  quads.push_back(quad);
}

void
BatchRenderer::DrawText(const char* text,
                        Vector2 position,
                        float fontSize,
                        Vector4 color,
                        int32_t drawOrder = 0.0f)
{
  // convert to ndc
  float posX = (position.x / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
  float posY = 1.0f - (position.y / static_cast<float>(windowHeight)) * 2.0f;

  float scale = fontSize / 90.0f; // note (David): 90 is baked font size

  for (const char* p = text; *p; ++p) {
    if (*p == '\n') {
      posY += fontSize;
      posX = position.x;
      continue;
    }

    char c = *p;
    if (c < 32 || c >= 128)
      continue;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(fontData.cdata, 512, 512, c - 32, &posX, &posY, &q, 1);

    float x0 = q.x0 * scale;
    float y0 = q.y0 * scale;
    float x1 = q.x1 * scale;
    float y1 = q.y1 * scale;

    float w = x1 - x0;
    float h = y1 - y0;

    Vector2 center = { position.x + x0 + w / 2.0f, position.y + y0 + h / 2.0f };

    float texCoords[4][2] = {
      { q.s0, q.t1 }, // bottom-left
      { q.s1, q.t1 }, // bottom-right
      { q.s1, q.t0 }, // top-right
      { q.s0, q.t0 }, // top-left
    };

    AddTexturedQuad(
      center, w, h, texCoords, 2, color, 0.0f, ORIGIN_CENTER, drawOrder);
  }
}

Vector2
BatchRenderer::MeasureText(const char* text, float fontSize)
{
  float posX = 0.0f;
  float posY = fontSize;

  float maxWidth = 0.0f;
  float scale = fontSize / 90.0f; // move backed font size somewhere else

  for (const char* p = text; *p; ++p) {
    if (*p == '\n') {
      posY += fontSize; // move down by the font size (line height)
      posX = 0.0f;      // reset X to the starting X position
      continue;
    }

    char c = *p;
    if (c < 32 || c >= 128)
      continue;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(fontData.cdata, 512, 512, c - 32, &posX, &posY, &q, 1);

    float x0 = q.x0 * scale;
    float x1 = q.x1 * scale;

    float width = x1 - x0;
    posX += width;

    if (posX > maxWidth) {
      maxWidth = posX;
    }
  }

  return Vector2{ maxWidth, posY };
}

void
BatchRenderer::Render(WGPURenderPassEncoder passEncoder)
{
  if (quads.empty())
    return;

  // sorting the quads based on drawOrder (ascending order)
  std::stable_sort(
    quads.begin(), quads.end(), [](const Quad& a, const Quad& b) {
      return a.drawOrder < b.drawOrder;
    });

  vertices.clear();
  indices.clear();
  numQuads = 0;

  // batch sorted quads
  for (const auto& quad : quads) {
    uint16_t baseIndex = static_cast<uint16_t>(numQuads * 4);
    for (auto index : quad.indices) {
      indices.push_back(baseIndex + index);
    }
    vertices.insert(vertices.end(), quad.vertices.begin(), quad.vertices.end());
    numQuads++;
  }

  // upload data to GPU buffers
  wgpuQueueWriteBuffer(
    queue, vertexBuffer, 0, vertices.data(), vertices.size() * sizeof(Vertex));
  wgpuQueueWriteBuffer(
    queue, indexBuffer, 0, indices.data(), indices.size() * sizeof(uint16_t));

  // update uniforms
  currentUniforms.uTime = static_cast<float>(SDL_GetTicks()) / 1000.0f;

  float left = 0.0f;
  float right = static_cast<float>(windowWidth);
  float bottom = static_cast<float>(windowHeight);
  float top = 0.0f;
  float near = -1.0f;
  float far = 1.0f;

  currentUniforms.uProjection =
    matrix4_orthographic(left, right, bottom, top, near, far);

  wgpuQueueWriteBuffer(
    queue, uniformBuffer, 0, &currentUniforms, sizeof(Uniforms));

  // set pipeline and buffers
  wgpuRenderPassEncoderSetPipeline(passEncoder, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(
    passEncoder, 0, vertexBuffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(
    passEncoder, indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);

  // set bind group for textures
  wgpuRenderPassEncoderSetBindGroup(passEncoder, 0, bindGroup, 0, nullptr);

  // draw quads
  wgpuRenderPassEncoderDrawIndexed(passEncoder, numQuads * 6, 1, 0, 0, 0);

  // reset for next frame
  quads.clear();
}

void
TextureSpriteTexCoords(SpriteFrameDesc desc, float texCoords[4][2])
{
  Texture tex = desc.texture;

  float u0 = (desc.sprite_coord.x * desc.sprite_size.x) / tex.width;
  float v0 = (desc.sprite_coord.y * desc.sprite_size.y) / tex.height;
  float u1 = ((desc.sprite_coord.x + 1) * desc.sprite_size.x) / tex.width;
  float v1 = ((desc.sprite_coord.y + 1) * desc.sprite_size.y) / tex.height;

  texCoords[0][0] = u0;
  texCoords[0][1] = v1; // bottom-left
  texCoords[1][0] = u1;
  texCoords[1][1] = v1; // bottom-right
  texCoords[2][0] = u1;
  texCoords[2][1] = v0; // top-right
  texCoords[3][0] = u0;
  texCoords[3][1] = v0; // top-left
}

void
DrawBoundingBox(BatchRenderer& renderer,
                const BoundingBox& bbox,
                Vector4 color,
                float rotation,
                int32_t drawOrder)
{
  float lineThickness = 3.0f;

  // Top Edge
  renderer.AddLine({ bbox.min.x, bbox.min.y },
                   { bbox.max.x, bbox.min.y },
                   lineThickness,
                   color,
                   rotation,
                   ORIGIN_CENTER,
                   drawOrder);
  // Right Edge
  renderer.AddLine({ bbox.max.x, bbox.min.y },
                   { bbox.max.x, bbox.max.y },
                   lineThickness,
                   color,
                   rotation,
                   ORIGIN_CENTER,
                   drawOrder);
  // Bottom Edge
  renderer.AddLine({ bbox.max.x, bbox.max.y },
                   { bbox.min.x, bbox.max.y },
                   lineThickness,
                   color,
                   rotation,
                   ORIGIN_CENTER,
                   drawOrder);
  // Left Edge
  renderer.AddLine({ bbox.min.x, bbox.max.y },
                   { bbox.min.x, bbox.min.y },
                   lineThickness,
                   color,
                   rotation,
                   ORIGIN_CENTER,
                   drawOrder);
}
