#pragma once

#include "../../Math.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "webgpu/webgpu.h"
#include "wgpu/wgpu.h"
#include <stdint.h>
#include <vector>

#include "stb/stb_image.h"
#include "stb/stb_truetype.h"

// clang-format off
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

#define ORIGIN_TOP_LEFT   { 0.0f, 0.0f }
#define ORIGIN_CENTER     { 0.5f, 0.5f }
#define ORIGIN_BOTTOM_RIGHT { 1.0f, 1.0f }
#define ORIGIN_BOTTOM_LEFT { -1.0f, 1.0f }
// clang-format on

enum Layer
{
  LAYER_BACKGROUND = 0,
  LAYER_GAME_OBJECT = 1,
  LAYER_UI = 2,
  LAYER_TEXT = 3
};

struct Vertex
{
  Vector2 position; // relative to the origin
  Vector4 color;
  Vector2 texCoord;
  float rotation; // angle in radians
  uint32_t texIndex;
  Vector2 translation; // per quad translation
  float padding;       // align to 16 bytes
};

struct Uniforms
{
  float uTime;
  float padding[3]; // enforce 16-byte alignment
  Matrix4 uProjection;
};

struct Texture
{
  WGPUTexture texture;
  WGPUTextureView textureView;
  WGPUSampler sampler;
  int32_t width;
  int32_t height;
};

struct SpriteFrameDesc
{
  Texture texture;
  Vector2 sprite_coord;
  Vector2 sprite_size;
};

struct Quad
{
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  int32_t drawOrder;
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

  void AddQuad(Vector2 position,
               float width,
               float height,
               Vector4 color,
               float rotation,
               Vector2 origin,
               int32_t drawOrder);

  void AddTexturedQuad(Vector2 position,
                       float width,
                       float height,
                       float texCoords[4][2],
                       uint32_t texIndex,
                       Vector4 color,
                       float rotation,
                       Vector2 origin,
                       int32_t drawOrder);

  void AddLine(Vector2 start,
               Vector2 end,
               float thickness,
               Vector4 color,
               float rotation,
               Vector2 origin,
               int32_t drawOrder);

  void DrawText(const char* text,
                Vector2 position,
                float fontSize,
                Vector4 color,
                int32_t drawOrder);

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
  WGPUBuffer uniformBuffer;
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
  Uniforms currentUniforms;

  std::vector<Quad> quads;

  const uint32_t MAX_QUADS = 10000;

public:
  struct Font
  {
    stbtt_fontinfo fontInfo;
    uint8_t* fontBuffer;
    uint8_t* bitmap;
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs (sorry but no
                               // Georgian lang support for now...)
  } fontData;
};

Vector4
RGBA32(uint32_t hexColor);
void
TextureSpriteTexCoords(SpriteFrameDesc desc, float texCoords[4][2]);

void
DrawBoundingBox(BatchRenderer& renderer,
                const BoundingBox& bbox,
                Vector4 color,
                float rotation,
                int32_t drawOrder);
