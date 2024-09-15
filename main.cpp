/**
 * $author David Kviloria <david@skystargames.com>
 * Simple 2D batch renderer implemented in WebGPU Native Backend.
 *
 * $file main.cpp
 */
#include <cstring>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "webgpu/webgpu.h"
#include "wgpu/wgpu.h"

#include "math.h"
#include <X11/Xlib.h>

#define WHITE { 1.0f, 1.0f, 1.0f, 1.0f }
#define BLACK { 0.0f, 0.0f, 0.0f, 1.0f }
#define RED { 1.0f, 0.0f, 0.0f, 1.0f }
#define GREEN { 0.0f, 1.0f, 0.0f, 1.0f }
#define BLUE { 0.0f, 0.0f, 1.0f, 1.0f }
#define YELLOW { 1.0f, 1.0f, 0.0f, 1.0f }
#define CYAN { 0.0f, 1.0f, 1.0f, 1.0f }
#define MAGENTA { 1.0f, 0.0f, 1.0f, 1.0f }
#define ORANGE { 1.0f, 0.5f, 0.0f, 1.0f }
#define PURPLE { 0.5f, 0.0f, 0.5f, 1.0f }
#define GREY { 0.5f, 0.5f, 0.5f, 1.0f }
#define PINK { 1.0f, 0.75f, 0.8f, 1.0f }
#define BROWN { 0.6f, 0.4f, 0.2f, 1.0f }
#define VIOLET { 0.56f, 0.0f, 1.0f, 1.0f }
#define LIME { 0.75f, 1.0f, 0.0f, 1.0f }
#define TEAL { 0.0f, 0.5f, 0.5f, 1.0f }
#define NAVY { 0.0f, 0.0f, 0.5f, 1.0f }
#define MAROON { 0.5f, 0.0f, 0.0f, 1.0f }
#define OLIVE { 0.5f, 0.5f, 0.0f, 1.0f }
#define AQUA { 0.0f, 1.0f, 1.0f, 1.0f }
#define GOLD { 1.0f, 0.84f, 0.0f, 1.0f }
#define SILVER { 0.75f, 0.75f, 0.75f, 1.0f }
#define BEIGE { 0.96f, 0.96f, 0.86f, 1.0f }
#define CORAL { 1.0f, 0.5f, 0.31f, 1.0f }
#define INDIGO { 0.29f, 0.0f, 0.51f, 1.0f }
#define TURQUOISE { 0.25f, 0.88f, 0.82f, 1.0f }

struct Vertex
{
  Vector2 position;
  Vector4 color;
  Vector2 texCoord;
  uint32_t texIndex;
};

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

// note (David): this currently working only for X11.
WGPUSurface
SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window)
{
  WGPUSurface surface = nullptr;

  if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
    Display* xdisplay =
      (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                       SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                                       NULL);
    Window xwindow = (Window)SDL_GetNumberProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (xdisplay && xwindow) {

      WGPUChainedStruct nexInChain = {};
      nexInChain.next = NULL;
      nexInChain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;

      WGPUSurfaceDescriptorFromXlibWindow fromXlibWind = {};
      fromXlibWind.chain = nexInChain;
      fromXlibWind.display = xdisplay;
      fromXlibWind.window = xwindow;

      WGPUSurfaceDescriptor surfaceDesc = {};
      surfaceDesc.label = NULL;
      surfaceDesc.nextInChain = (const WGPUChainedStruct*)&fromXlibWind;

      surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
  }

  return surface;
}

struct Texture
{
  WGPUTexture texture;
  WGPUTextureView textureView;
  WGPUSampler sampler;
  int32_t width;
  int32_t height;
};

class BatchRenderer
{
public:
  BatchRenderer(WGPUDevice device,
                WGPUQueue queue,
                int32_t width,
                int32_t height);
  ~BatchRenderer();
  void Initialize();
  void LoadTexture(const char* filePath, int32_t textureIndex);
  void LoadFont(const char* fontFilePath);
  void AddQuad(Vector2 position, float width, float height, Vector4 color);
  void AddTexturedQuad(Vector2 position,
                       float width,
                       float height,
                       float texCoords[4][2],
                       uint32_t texIndex,
                       Vector4 color);
  void AddLine(Vector2 start, Vector2 end, float thickness, Vector4 color);
  void DrawText(const char* text,
                Vector2 position,
                float fontSize,
                Vector4 color);
  Vector2 MeasureText(const char* text, float fontSize);
  void Render(WGPURenderPassEncoder passEncoder);

  int32_t windowWidth;
  int32_t windowHeight;

private:
  WGPUDevice device;
  WGPUQueue queue;
  WGPURenderPipeline pipeline;
  WGPUBuffer vertexBuffer;
  WGPUBuffer indexBuffer;
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  size_t numQuads;

  void CreatePipeline();
  void CreateBuffers();
  void CreateBindGroup();

  WGPUShaderModule vertexShaderModule;
  WGPUShaderModule fragmentShaderModule;

  WGPUBindGroupLayout bindGroupLayout;
  WGPUBindGroup bindGroup;
  Texture textures[2];

  struct Font
  {
    stbtt_fontinfo fontInfo;
    uint8_t* fontBuffer;
    uint8_t* bitmap;
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs (sorry but no
                               // Georgian lang support for now...)
  } fontData;

public:
  float ConvertX(float x) const
  {
    return (x / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
  }
  float ConvertY(float y) const
  {
    return 1.0f - (y / static_cast<float>(windowHeight)) * 2.0f;
  }
};

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
  LoadFont("res/Alegreya-Regular.ttf"); // this will be bound to 1
  CreateBindGroup();
}

void
BatchRenderer::CreatePipeline()
{
  const char* vertexShaderCode = R"(
        struct VertexInput {
            @location(0) position : vec2<f32>,
            @location(1) color : vec4<f32>,
            @location(2) texCoord : vec2<f32>,
            @location(3) texIndex : u32,
        };

        struct VertexOutput {
            @builtin(position) position : vec4<f32>,
            @location(0) color : vec4<f32>,
            @location(1) texCoord : vec2<f32>,
            @location(2) texIndex : u32,
        };

        @vertex
        fn main(input : VertexInput) -> VertexOutput {
            var output : VertexOutput;
            output.position = vec4<f32>(input.position, 0.0, 1.0);
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

  // vertex attributes
  WGPUVertexAttribute attributes[4];
  attributes[0] = { WGPUVertexFormat_Float32x2, offsetof(Vertex, position), 0 };
  attributes[1] = { WGPUVertexFormat_Float32x4, offsetof(Vertex, color), 1 };
  attributes[2] = { WGPUVertexFormat_Float32x2, offsetof(Vertex, texCoord), 2 };
  attributes[3] = { WGPUVertexFormat_Uint32, offsetof(Vertex, texIndex), 3 };

  // vertex buffer layout
  WGPUVertexBufferLayout vertexBufferLayout = {};
  vertexBufferLayout.arrayStride = sizeof(Vertex);
  vertexBufferLayout.attributeCount = 4;
  vertexBufferLayout.attributes = attributes;
  vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

  // bind group layout
  WGPUBindGroupLayoutEntry bglEntries[4] = {};
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

  WGPUBindGroupLayoutDescriptor bglDesc = {};
  bglDesc.entryCount = 4;
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
  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = false;

  pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

  wgpuPipelineLayoutRelease(pipelineLayout);
}

void
BatchRenderer::CreateBuffers()
{
  const size_t maxVertices = 1000 * 4;
  const size_t maxIndices = 1000 * 6;

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
  fread(fontData.fontBuffer, 1, fontSize, fontFile);
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
  WGPUBindGroupEntry bgEntries[4] = {};

  // texture 0
  bgEntries[0].binding = 0;
  bgEntries[0].textureView = textures[0].textureView;

  bgEntries[1].binding = 1;
  bgEntries[1].sampler = textures[0].sampler;

  // texture 1 (Font)
  bgEntries[2].binding = 2;
  bgEntries[2].textureView = textures[1].textureView;

  bgEntries[3].binding = 3;
  bgEntries[3].sampler = textures[1].sampler;

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.layout = bindGroupLayout;
  bgDesc.entryCount = 4;
  bgDesc.entries = bgEntries;

  bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void
BatchRenderer::AddQuad(Vector2 position,
                       float width,
                       float height,
                       Vector4 color)
{
  if (numQuads >= 1000)
    return;

  float halfWidth = width / 2.0f;
  float halfHeight = height / 2.0f;

  float left = position.x - halfWidth;
  float right = position.x + halfWidth;
  float top = position.y - halfHeight;
  float bottom = position.y + halfHeight;

  Vertex v0 = { { ConvertX(left), ConvertY(bottom) },
                { color.x, color.y, color.z, color.w },
                { 0.0f, 1.0f },
                0u };
  Vertex v1 = { { ConvertX(right), ConvertY(bottom) },
                { color.x, color.y, color.z, color.w },
                { 1.0f, 1.0f },
                0u };
  Vertex v2 = { { ConvertX(right), ConvertY(top) },
                { color.x, color.y, color.z, color.w },
                { 1.0f, 0.0f },
                0u };
  Vertex v3 = { { ConvertX(left), ConvertY(top) },
                { color.x, color.y, color.z, color.w },
                { 0.0f, 0.0f },
                0u };

  vertices.insert(vertices.end(), { v0, v1, v2, v3 });

  uint16_t baseIndex = numQuads * 4;
  indices.insert(indices.end(),
                 { baseIndex,
                   (uint16_t)(baseIndex + 1),
                   (uint16_t)(baseIndex + 2),
                   baseIndex,
                   (uint16_t)(baseIndex + 2),
                   (uint16_t)(baseIndex + 3) });

  numQuads++;
}

void
BatchRenderer::AddTexturedQuad(

  Vector2 position,
  float width,
  float height,
  float texCoords[4][2],
  uint32_t texIndex,
  Vector4 color)
{
  if (numQuads >= 1000)
    return;

  float halfWidth = width / 2.0f;
  float halfHeight = height / 2.0f;

  float left = position.x - halfWidth;
  float right = position.x + halfWidth;
  float top = position.y - halfHeight;
  float bottom = position.y + halfHeight;

  Vertex v0 = { { ConvertX(left), ConvertY(bottom) },
                { color.x, color.y, color.z, color.w },
                { texCoords[0][0], texCoords[0][1] },
                texIndex };
  Vertex v1 = { { ConvertX(right), ConvertY(bottom) },
                { color.x, color.y, color.z, color.w },
                { texCoords[1][0], texCoords[1][1] },
                texIndex };
  Vertex v2 = { { ConvertX(right), ConvertY(top) },
                { color.x, color.y, color.z, color.w },
                { texCoords[2][0], texCoords[2][1] },
                texIndex };
  Vertex v3 = { { ConvertX(left), ConvertY(top) },
                { color.x, color.y, color.z, color.w },
                { texCoords[3][0], texCoords[3][1] },
                texIndex };

  vertices.insert(vertices.end(), { v0, v1, v2, v3 });

  uint16_t baseIndex = numQuads * 4;
  indices.insert(indices.end(),
                 { baseIndex,
                   (uint16_t)(baseIndex + 1),
                   (uint16_t)(baseIndex + 2),
                   baseIndex,
                   (uint16_t)(baseIndex + 2),
                   (uint16_t)(baseIndex + 3) });

  numQuads++;
}

void
BatchRenderer::AddLine(Vector2 start,
                       Vector2 end,
                       float thickness,
                       Vector4 color)
{
  if (numQuads >= 1000)
    return;

  Vector2 dir = { end.x - start.x, end.y - start.y };
  float length = sqrt(dir.x * dir.x + dir.y * dir.y);

  if (length == 0.0f)
    return; // cannot create a line with zero length

  Vector2 normDir = { dir.x / length, dir.y / length };

  Vector2 perp = { -normDir.y, normDir.x };

  float halfThickness = thickness / 2.0f;
  Vector2 offset = { perp.x * halfThickness, perp.y * halfThickness };

  Vector2 v1 = { start.x + offset.x, start.y + offset.y };
  Vector2 v2 = { start.x - offset.x, start.y - offset.y };
  Vector2 v3 = { end.x - offset.x, end.y - offset.y };
  Vector2 v4 = { end.x + offset.x, end.y + offset.y };

  Vertex vertex0 = { { ConvertX(v1.x), ConvertY(v1.y) },
                     { color.x, color.y, color.z, color.w },
                     { 0.0f, 0.0f }, // texCoord not used for color quads
                     0u };
  Vertex vertex1 = { { ConvertX(v2.x), ConvertY(v2.y) },
                     { color.x, color.y, color.z, color.w },
                     { 0.0f, 0.0f },
                     0u };
  Vertex vertex2 = { { ConvertX(v3.x), ConvertY(v3.y) },
                     { color.x, color.y, color.z, color.w },
                     { 0.0f, 0.0f },
                     0u };
  Vertex vertex3 = { { ConvertX(v4.x), ConvertY(v4.y) },
                     { color.x, color.y, color.z, color.w },
                     { 0.0f, 0.0f },
                     0u };

  vertices.insert(vertices.end(), { vertex0, vertex1, vertex2, vertex3 });
  uint16_t baseIndex = static_cast<uint16_t>(numQuads * 4);

  indices.insert(indices.end(),
                 { baseIndex,
                   static_cast<uint16_t>(baseIndex + 1),
                   static_cast<uint16_t>(baseIndex + 2),
                   baseIndex,
                   static_cast<uint16_t>(baseIndex + 2),
                   static_cast<uint16_t>(baseIndex + 3) });

  numQuads++;
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
BatchRenderer::DrawText(const char* text,
                        Vector2 position,
                        float fontSize,
                        Vector4 color)
{
  float posX = position.x;
  float posY = position.y;

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

    // Center of the quad
    Vector2 center = { position.x + x0 + w / 2.0f, position.y + y0 + h / 2.0f };

    float texCoords[4][2] = {
      { q.s0, q.t1 }, // bottom-left
      { q.s1, q.t1 }, // bottom-right
      { q.s1, q.t0 }, // top-right
      { q.s0, q.t0 }, // top-left
    };

    AddTexturedQuad(center, w, h, texCoords, 2, color);
  }
}

void
BatchRenderer::Render(WGPURenderPassEncoder passEncoder)
{
  if (numQuads == 0)
    return;

  // update buffers
  wgpuQueueWriteBuffer(
    queue, vertexBuffer, 0, vertices.data(), vertices.size() * sizeof(Vertex));
  wgpuQueueWriteBuffer(
    queue, indexBuffer, 0, indices.data(), indices.size() * sizeof(uint16_t));

  // set pipeline and buffers
  wgpuRenderPassEncoderSetPipeline(passEncoder, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(
    passEncoder, 0, vertexBuffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(
    passEncoder, indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);

  // set bind group for textures
  wgpuRenderPassEncoderSetBindGroup(passEncoder, 0, bindGroup, 0, nullptr);

  // draw
  wgpuRenderPassEncoderDrawIndexed(passEncoder, numQuads * 6, 1, 0, 0, 0);

  // reset data for next frame
  vertices.clear();
  indices.clear();
  numQuads = 0;
}

struct SpriteFrameDesc
{
  Texture texture;
  Vector2 sprite_coord;
  Vector2 sprite_size;
};

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
DrawBoundingBox(BatchRenderer& renderer, const BoundingBox& bbox, Vector4 color)
{
  // Top Edge
  renderer.AddLine(
    { bbox.min.x, bbox.min.y }, { bbox.max.x, bbox.min.y }, 1.0f, color);
  // Right Edge
  renderer.AddLine(
    { bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.max.y }, 1.0f, color);
  // Bottom Edge
  renderer.AddLine(
    { bbox.max.x, bbox.max.y }, { bbox.min.x, bbox.max.y }, 1.0f, color);
  // Left Edge
  renderer.AddLine(
    { bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.min.y }, 1.0f, color);
}

Vector2 mouse = {};
Vector2 global_mouse = {};

int
main(int32_t argc, char* argv[])
{
  SDL_Window* window =
    SDL_CreateWindow("A Window", 800, 600, SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cout << "Window Creation Failed\n";
    return -1;
  }

  WGPUInstanceDescriptor instanceDescriptor = {};
  instanceDescriptor.nextInChain = nullptr;
  WGPUInstance instance = wgpuCreateInstance(&instanceDescriptor);
  if (!instance) {
    std::cout << "Could not create WGPU instance\n";
    return 1;
  }

  WGPUSurface surface = SDL_GetWGPUSurface(instance, window);

  WGPURequestAdapterOptions adapterOptions = {};
  adapterOptions.compatibleSurface = surface;

  WGPUAdapter adapter = nullptr;
  auto requestAdapterCallback = [](WGPURequestAdapterStatus status,
                                   WGPUAdapter receivedAdapter,
                                   const char* message,
                                   void* userdata) {
    if (status == WGPURequestAdapterStatus_Success) {
      *(WGPUAdapter*)userdata = receivedAdapter;
    } else {
      std::cout << "Failed to get WGPU Adapter: " << message << "\n";
    }
  };

  wgpuInstanceRequestAdapter(
    instance, &adapterOptions, requestAdapterCallback, &adapter);

  if (!adapter) {
    std::cout << "Unable to request adapter\n";
    return 1;
  }

  WGPUDevice device = nullptr;
  auto deviceRequestCallback = [](WGPURequestDeviceStatus status,
                                  WGPUDevice receivedDevice,
                                  const char* message,
                                  void* userdata) {
    if (status == WGPURequestDeviceStatus_Success) {
      *(WGPUDevice*)userdata = receivedDevice;
    } else {
      std::cout << "Failed to get WGPU Device: " << message << "\n";
    }
  };

  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  deviceDesc.label = "My Device";

  wgpuAdapterRequestDevice(
    adapter, &deviceDesc, deviceRequestCallback, &device);

  if (!device) {
    std::cout << "Unable to request device\n";
    return 1;
  }

  WGPUQueue commandQueue = wgpuDeviceGetQueue(device);

  // surface configuration
  WGPUSurfaceConfiguration config = {};
  config.nextInChain = nullptr;
  config.format = WGPUTextureFormat_BGRA8Unorm;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.device = device;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Opaque;
  config.width = 800;
  config.height = 600;
  wgpuSurfaceConfigure(surface, &config);

  // surface texture view descriptor
  WGPUTextureViewDescriptor viewDescriptor = {};
  viewDescriptor.nextInChain = nullptr;
  viewDescriptor.label = "Surface texture view";
  viewDescriptor.format = WGPUTextureFormat_BGRA8Unorm;
  viewDescriptor.dimension = WGPUTextureViewDimension_2D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = WGPUTextureAspect_All;

  bool is_running = true;
  SDL_Event e;

  BatchRenderer batchRenderer(
    device, commandQueue, config.width, config.height);
  batchRenderer.Initialize();

  Vector2 position = { 400.0f, 300.0f };

  const float speed = 200.0f;

  Uint64 lastTime = SDL_GetPerformanceCounter();
  float deltaTime = 0.0f;

  float texCoords[4][2] = {};

  SpriteFrameDesc spriteDesc = {};
  spriteDesc.sprite_size = { 16.0f, 16.0f };
  spriteDesc.sprite_coord = { 17.0f, 31.0f };
  spriteDesc.texture.width = 512;
  spriteDesc.texture.height = 512;

  TextureSpriteTexCoords(spriteDesc, texCoords);

  int32_t frameCount = 0;
  float fps = 0.0f;
  float fpsTimer = 0.0f;

  while (is_running) {

    Uint64 currentTime = SDL_GetPerformanceCounter();
    deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
    lastTime = currentTime;

    // Update FPS Timer and Frame Count
    fpsTimer += deltaTime;
    frameCount++;

    if (fpsTimer >= 1.0f) { // Every second
      fps = frameCount / fpsTimer;
      frameCount = 0;
      fpsTimer -= 1.0f; // Subtract 1 second to handle any extra time
    }

    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_EVENT_QUIT:
          is_running = false;
          break;

        case SDL_EVENT_WINDOW_RESIZED: {
          int32_t newWidth, newHeight;
          SDL_GetWindowSize(window, &newWidth, &newHeight);
          batchRenderer.windowWidth = newWidth;
          batchRenderer.windowHeight = newHeight;
          config.width = newWidth;
          config.height = newHeight;
          wgpuSurfaceConfigure(surface, &config);
        } break;

        case SDL_EVENT_KEY_DOWN: {
          if (e.key.key == SDLK_ESCAPE) {
            is_running = false;
          }
        } break;

        default:
          break;
      }
    }

    // UPDATE -------------------------------------------------
    Uint32 mouseButtons =
      SDL_GetGlobalMouseState(&global_mouse.x, &global_mouse.y);
    mouseButtons = SDL_GetMouseState(&mouse.x, &mouse.y);

    const SDL_bool* keyboardState = SDL_GetKeyboardState(NULL);
    if (keyboardState[SDL_SCANCODE_A]) {
      position.x -= speed * deltaTime;
    }
    if (keyboardState[SDL_SCANCODE_D]) {
      position.x += speed * deltaTime;
    }
    if (keyboardState[SDL_SCANCODE_W]) {
      position.y -= speed * deltaTime;
    }
    if (keyboardState[SDL_SCANCODE_S]) {
      position.y += speed * deltaTime;
    }

    if (position.x < 0)
      position.x = 0;
    if (position.x > batchRenderer.windowWidth)
      position.x = batchRenderer.windowWidth;
    if (position.y < 0)
      position.y = 0;
    if (position.y > batchRenderer.windowHeight)
      position.y = batchRenderer.windowHeight;

    // RENDER -----------------------------------------------

    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (!surfaceTexture.texture) {
      std::cout << "Unable to get texture from surface.\n";
      continue;
    }

    WGPUTextureView targetView =
      wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(device, nullptr);

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = targetView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = { 0.1, 0.1, 0.1, 1.0 };

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;
    renderPassDesc.nextInChain = nullptr;

    WGPURenderPassEncoder passEncoder =
      wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    static Vector4 color = RED;
    BoundingBox playerBB =
      CreateBoundingBoxFromCenter(&position, 100.0f, 100.0f);
    if (IsPointInside(&playerBB, &mouse)) {
      color = GREEN;
    } else {
      color = RED;
    }

    static float rotationAngle = 100.0f;
    float rotationSpeed = 15.0f;
    float distanceFromQuad = 10.0f;

    batchRenderer.AddQuad(position, 100, 100, color);

    char dbuffer[255];
    sprintf(dbuffer, "%f, %f", position.x, position.y);

    rotationAngle += rotationSpeed * deltaTime;
    if (rotationAngle > 2.0f * M_PI) {
      rotationAngle -= 2.0f * M_PI;
    }

    Vector2 quadCenter = { position.x, position.y };
    float textX = quadCenter.x + distanceFromQuad * cos(rotationAngle);
    float textY = quadCenter.y + distanceFromQuad * sin(rotationAngle);
    Vector2 textPosition = { textX, textY };

    batchRenderer.DrawText(dbuffer, textPosition, 30.0f, BLUE);

    batchRenderer.AddQuad(textPosition, 50, 50, GREEN);
    batchRenderer.AddQuad({ 600, 450 }, 150, 75, BLUE);

    batchRenderer.AddLine({ 100.0f, 300.0f }, { 700.0f, 300.0f }, 10.0f, RED);
    batchRenderer.AddLine({ 400.0f, 100.0f }, { 400.0f, 500.0f }, 10.0f, GREEN);
    batchRenderer.AddLine({ 100.0f, 100.0f }, { 700.0f, 500.0f }, 10.0f, BLUE);

    batchRenderer.AddTexturedQuad({ 400, 300 }, 200, 200, texCoords, 1, WHITE);
    batchRenderer.DrawText("Hello, WebGPU!", { 100.0f, 100.0f }, 90.0f, WHITE);

    char buffer[255];
    sprintf(buffer, "Global: %.0f,%.0f", global_mouse.x, global_mouse.y);
    batchRenderer.DrawText(buffer, { 10, 30 }, 30.0f, WHITE);

    buffer[0] = 0;

    sprintf(buffer, "Relative: %.0f, %.0f", mouse.x, mouse.y);
    batchRenderer.DrawText(buffer, { 10, 60 }, 30.0f, WHITE);

    DrawBoundingBox(batchRenderer, playerBB, YELLOW);

    char fpsBuffer[64];
    sprintf(
      fpsBuffer, "FPS: %.0f / Frame Time: %.3f ms", fps, deltaTime * 1000.0f);
    Vector2 text_size = batchRenderer.MeasureText(fpsBuffer, 30.0f);

    batchRenderer.DrawText(fpsBuffer, { 10.0f, 500 - 90 }, 30.0f, WHITE);

    batchRenderer.Render(passEncoder);

    wgpuRenderPassEncoderEnd(passEncoder);
    wgpuRenderPassEncoderRelease(passEncoder);

    // we are finishing encoding and submitting
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    WGPUCommandBuffer commandBuffer =
      wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuQueueSubmit(commandQueue, 1, &commandBuffer);

    // finally, preset the fucking frame
    wgpuSurfacePresent(surface);

    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(targetView);
    wgpuTextureRelease(surfaceTexture.texture);
  }

  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  wgpuInstanceRelease(instance);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
