/**
 * $author David Kviloria <david@skystargames.com>
 * Simple 2D batch renderer implemented in WebGPU Native Backend.
 *
 * $file main.cpp
 */

#include "pch.h"

#include "Math.h"
#include "backend/2d/Renderer.h"
#include "backend/common.h"

#include "Editor.h"
#include "Imui.h"
#include "Platform.h"
#include "PlayMath.h"
#include "widget/PerfVisualizer.h"

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720

#define EDITOR_NAME "DKEDIT"

#include "CommandPallete.h"

int
main(int32_t argc, char* argv[])
{
  SDL_Window* window = SDL_CreateWindow(
    EDITOR_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cout << "Window Creation Failed\n";
    return -1;
  }

  SDL_SetWindowBordered(window, SDL_FALSE);

  SDL_Surface* icon = SDL_LoadBMP("./res/icon.bmp");
  if (icon != NULL) {
    SDL_SetWindowIcon(window, icon);
    SDL_DestroySurface(icon);
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
  WGPUSurfaceCapabilities capabilities;
  wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);

  WGPUSurfaceConfiguration config = {};
  config.nextInChain = nullptr;
  config.format = WGPUTextureFormat_BGRA8Unorm; // capabilities.formats[1];
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.device = device;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Opaque;
  config.width = WINDOW_WIDTH;
  config.height = WINDOW_HEIGHT;
  wgpuSurfaceConfigure(surface, &config);

  wgpuSurfaceCapabilitiesFreeMembers(capabilities);

  // surface texture view descriptor
  WGPUTextureViewDescriptor viewDescriptor = {};
  viewDescriptor.nextInChain = nullptr;
  viewDescriptor.label = "Surface texture view";
  viewDescriptor.format = config.format;
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

  UIContext uiContext = {};

#if 0
  CartesianCoordinateSystem coordinateSystem = {
    .origin = { (float)config.width * 0.5f, (float)config.height * 0.5f },
    .screenSize = { (float)config.width, (float)config.height },
    .scale = 50.0f, // pixels per unit
    .axisColor = BLACK,
    .gridColor = RGBA32(0x4E4D5D),
    .textColor = WHITE
  };

  Vector2 vectorStart = { 400.0f, 300.0f };
  float vectorLength = 50.0f;
  Vector2 vectorToVisualize = { vectorLength, 0.0f };

  ControlPoint p0 = { { 100, 500 }, false };
  ControlPoint p1 = { { 400, 100 }, false };
  ControlPoint p2 = { { 700, 500 }, false };

  p0 = { { 400, 300 }, false };
  p1 = { { 200, 100 }, false };
  p2 = { { 600, 100 }, false };
  ControlPoint p3 = { { 700, 300 }, false };
#endif

  float graphWidth = 200.0f;
  float graphHeight = 20.0f;
  int maxSegments = 30;
  Vector2 graphOrigin = { 10.0f,
                          config.height - ((graphHeight * 0.5f) - 10.0f) };

  PerfVisualizer perfVisualizer(
    batchRenderer, graphOrigin, graphWidth, graphHeight, maxSegments);

  Vector4 lineColor = { 0.0f, 1.0f, 0.0f, 1.0f };
  Vector4 spikeColor = { 1.0f, 0.0f, 0.0f, 1.0f };

  Vector2 editor_position = { 10.0f, 50.0f };
  float fontSize = 23.0f;
  Vector4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
  Vector4 cursorColor = LIME;
  Vector4 selectionColor = { 0.0f, 0.5f, 1.0f, 0.5f };
  Vector4 lineNumberColor = { 0.7f, 0.7f, 0.7f, 1.0f };

  SimpleTextEditor editor(batchRenderer,
                          editor_position,
                          fontSize,
                          textColor,
                          cursorColor,
                          selectionColor,
                          lineNumberColor);

  SDL_StartTextInput(window);

  if (argc > 1) {
    const char* filename = argv[1];
    editor.loadTextFromFile(filename);
    char buffer[255];
    sprintf(buffer, "%s | %s", EDITOR_NAME, filename);
    SDL_SetWindowTitle(window, buffer);
  }

  CommandPalette commandPalette(batchRenderer, config.width, config.height);
  commandPalette.onItemSelect = [&](const CommandPalette::Item& item) {
    switch (commandPalette.getMode()) {
      case CommandPaletteMode::FileList:
        editor.loadTextFromFile(item.displayText.c_str());
        {
          char buffer[255];
          sprintf(buffer, "%s | %s", EDITOR_NAME, item.displayText.c_str());
          SDL_SetWindowTitle(window, buffer);
        }
        break;
      case CommandPaletteMode::CommentList:
      case CommandPaletteMode::FunctionList:
        editor.handleCommandPaletteSelection(item.data);
        break;
      case CommandPaletteMode::SystemCommand:
        break;
    }
  };

  commandPalette.onCommandSelect = [&](const std::string& command) {
    if (command == "/q") {
      std::cout << "Exiting."
                << "\n";
      SDL_Event e;
      e.type = SDL_EVENT_QUIT;
      SDL_PushEvent(&e);
    } else if (command.substr(0, 1) == "/n") {
      std::string filename = command.substr(1);
      std::cout << "Creating new file " << filename << "\n";
    } else if (command == "/w") {
      std::cout << "Saving buffer (not implemented use Ctr+S)."
                << "\n";
      editor.saveBufferToFile();
    } else if (command == "/r") {
      editor.loadProjectConfig();
    } else if (command == "/fmt") {
      editor.formatCodeWithClangFormat();
    }
  };

  while (is_running) {
    Uint64 currentTime = SDL_GetPerformanceCounter();
    deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
    lastTime = currentTime;

    fpsTimer += deltaTime;
    frameCount++;

    if (fpsTimer >= 1.0f) {
      fps = frameCount / fpsTimer;
      frameCount = 0;
      fpsTimer -= 1.0f;
    }

    while (SDL_PollEvent(&e)) {
      bool ctrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;

      if (commandPalette.isVisible()) {
        commandPalette.handleInput(e);
      } else {
        editor.handleInput(e);

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
            editor.resize(config.width, config.height);
            perfVisualizer.Resize(config.width, config.height);
            commandPalette.resize(config.width, config.height);
          } break;

          case SDL_EVENT_KEY_DOWN: {
#if 0
            if (e.key.key == SDLK_ESCAPE) {
              is_running = false;
            }
            else
#endif
            if (e.key.key == SDLK_P) {
              if (ctrlPressed) {
                commandPalette.show();
                commandPalette.setEditorText(editor.getText());
              }
            }

          } break;

          default:
            break;
        }
      }
    }

    // UPDATE -------------------------------------------------
    SDL_GetGlobalMouseState(&uiContext.Mouse.global.x,
                            &uiContext.Mouse.global.y);
    Uint32 mouseState = SDL_GetMouseState(&uiContext.Mouse.relative.x,
                                          &uiContext.Mouse.relative.y);
    uiContext.Mouse.pressed = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

#if 0
    HandleMouseDragging(p0, uiContext.Mouse.relative, uiContext.Mouse.pressed);
    HandleMouseDragging(p1, uiContext.Mouse.relative, uiContext.Mouse.pressed);
    HandleMouseDragging(p2, uiContext.Mouse.relative, uiContext.Mouse.pressed);
    HandleMouseDragging(p3, uiContext.Mouse.relative, uiContext.Mouse.pressed);
#endif

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

    position = vector2_lerp(position, uiContext.Mouse.relative, deltaTime);

    editor.update(deltaTime);

    // RENDER -----------------------------------------------
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (!surfaceTexture.texture) {
      std::cout << "Unable to get texture from surface.\n";
      continue;
    }

    WGPUTextureDescriptor multiSampledFrameDesc = {};
    multiSampledFrameDesc.label = "Multi-sampled texture";
    multiSampledFrameDesc.size.width = config.width;
    multiSampledFrameDesc.size.height = config.height;
    multiSampledFrameDesc.size.depthOrArrayLayers = 1;
    multiSampledFrameDesc.mipLevelCount = 1;
    multiSampledFrameDesc.sampleCount = MSAA_NUMBER_OF_SAMPLE;
    multiSampledFrameDesc.dimension = WGPUTextureDimension_2D;
    multiSampledFrameDesc.format = config.format;
    multiSampledFrameDesc.usage = WGPUTextureUsage_RenderAttachment;

    WGPUTexture multiSampledTexture =
      wgpuDeviceCreateTexture(device, &multiSampledFrameDesc);
    WGPUTextureView multiSampledTextureView =
      wgpuTextureCreateView(multiSampledTexture, nullptr);

    WGPUTextureView targetView =
      wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(device, nullptr);

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view =
      multiSampledTextureView; // multi-sample render target
    colorAttachment.resolveTarget = targetView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    Vector4 clearColor = RGBA32(0x2B2A33);
    colorAttachment.clearValue = {
      clearColor.x, clearColor.y, clearColor.z, clearColor.w
    };

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder passEncoder =
      wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    perfVisualizer.AddFPS(1.0f / deltaTime);
    perfVisualizer.Draw(lineColor, spikeColor);

    char fpsBuffer[64];
    sprintf(fpsBuffer, "FPS: %.0f / %.2f ms", fps, deltaTime * 1000.0f);
    Vector2 fps_text_position = { 10.0f - 2.0f, (config.height) - 2.0f };
    batchRenderer.DrawText(
      fpsBuffer, fps_text_position, 30.0f, BLACK, LAYER_TEXT);

    fps_text_position.x += 2.0f;
    fps_text_position.y += 2.0f;
    batchRenderer.DrawText(
      fpsBuffer, fps_text_position, 30.0f, WHITE, LAYER_TEXT);

#if 0
    {
      float dx = uiContext.Mouse.relative.x - vectorStart.x;
      float dy = uiContext.Mouse.relative.y - vectorStart.y;
      float newAngle = atan2f(dy, dx);

      vectorToVisualize.x = vectorLength * cos(newAngle);
      vectorToVisualize.y = vectorLength * sin(newAngle);

      DrawCartesianAxes(batchRenderer, coordinateSystem);
      DrawCartesianGrid(batchRenderer, coordinateSystem);
      DrawCartesianLabels(batchRenderer, coordinateSystem);
      Visualize2DVector(
        batchRenderer, vectorStart, vectorToVisualize, 3.0f, BLUE);

      // control points
      DrawControlPoint(batchRenderer, p0, 10.0f, RED);
      DrawControlPoint(batchRenderer, p1, 10.0f, GREEN);
      DrawControlPoint(batchRenderer, p2, 10.0f, YELLOW);
      DrawControlPoint(batchRenderer, p3, 10.0f, PURPLE);

      DrawQuadraticBezierControlHandles(
        batchRenderer, p0.position, p1.position, p2.position, YELLOW, 1.0f);
      DrawCubicBezierControlHandles(batchRenderer,
                                    p0.position,
                                    p1.position,
                                    p2.position,
                                    p3.position,
                                    YELLOW,
                                    1.0f);
      DrawQuadraticBezier(
        batchRenderer, p0.position, p1.position, p2.position, GREEN, 5.0f);
      DrawCubicBezier(batchRenderer,
                      p0.position,
                      p1.position,
                      p2.position,
                      p3.position,
                      BLUE,
                      5.0f);

      static Vector4 color = RED;
      BoundingBox playerBB =
        CreateBoundingBoxFromCenter(&position, 100.0f, 100.0f);
      if (IsPointInside(&playerBB, &uiContext.Mouse.relative)) {
        color = GREEN;
      } else {
        color = RED;
      }

      static float rotationAngle = 100.0f;
      float rotationSpeed = 15.0f;
      float distanceFromQuad = 10.0f;

      rotationAngle = rotationSpeed * deltaTime;
      if (rotationAngle > 2.0f * M_PI) {
        rotationAngle -= 2.0f * M_PI;
      }

      batchRenderer.AddQuad(
        position, 100, 100, color, 0.0f, ORIGIN_CENTER, LAYER_GAME_OBJECT);

      float angle =
        atan2f(uiContext.Mouse.relative.y, uiContext.Mouse.relative.x);

      Vector2 quadCenter = { position.x, position.y };
      float textX = quadCenter.x + distanceFromQuad * cos(rotationAngle);
      float textY = quadCenter.y + distanceFromQuad * sin(rotationAngle);
      Vector2 textPosition = { textX, textY };

      batchRenderer.AddQuad(
        position, 50, 50, GREEN, angle, ORIGIN_CENTER, LAYER_GAME_OBJECT);
      batchRenderer.AddQuad(
        { 600, 450 }, 150, 75, BLUE, -angle, ORIGIN_CENTER, LAYER_BACKGROUND);

      batchRenderer.AddLine({ 100.0f, 300.0f },
                            { 700.0f, 300.0f },
                            10.0f,
                            RED,
                            angle,
                            ORIGIN_CENTER,
                            LAYER_GAME_OBJECT);

      batchRenderer.AddLine({ 400.0f, 100.0f },
                            { 400.0f, 500.0f },
                            10.0f,
                            GREEN,
                            angle,
                            ORIGIN_CENTER,
                            LAYER_GAME_OBJECT);

      batchRenderer.AddLine({ 100.0f, 100.0f },
                            { 700.0f, 500.0f },
                            10.0f,
                            BLUE,
                            angle,
                            ORIGIN_CENTER,
                            LAYER_GAME_OBJECT);

      batchRenderer.AddTexturedQuad({ 400, 300 },
                                    200,
                                    200,
                                    texCoords,
                                    1,
                                    WHITE,
                                    angle,
                                    ORIGIN_CENTER,
                                    LAYER_BACKGROUND);

      DrawBoundingBox(
        batchRenderer, playerBB, YELLOW, angle, LAYER_GAME_OBJECT);

    const char* hello = "Hello, WebGPU!";
    Vector2 text_size = batchRenderer.MeasureText(fpsBuffer, 90.0f);
    batchRenderer.DrawText(hello,
                           { (WINDOW_WIDTH - text_size.x) - 2.0f,
                             (config.height - text_size.y * 0.5f) - 2.0f },
                           90.0f,
                           BLACK,
                           LAYER_TEXT);
    batchRenderer.DrawText(
      hello,
      { (WINDOW_WIDTH - text_size.x), config.height - text_size.y * 0.5f },
      90.0f,
      WHITE,
      LAYER_TEXT);

    char debug_text[64];
    sprintf(debug_text,
            "(%.0f,%.0f)",
            uiContext.Mouse.relative.x,
            uiContext.Mouse.relative.y);
    batchRenderer.DrawText(
      debug_text,
      { uiContext.Mouse.relative.x + 2.0f, uiContext.Mouse.relative.y + 2.0f },
      30.0f,
      BLACK,
      LAYER_TEXT);
    batchRenderer.DrawText(
      debug_text, uiContext.Mouse.relative, 30.0f, WHITE, LAYER_TEXT);
}
#endif

    editor.render(batchRenderer);
    if (commandPalette.isVisible()) {
      commandPalette.render();
    }

    batchRenderer.Render(passEncoder);

    wgpuRenderPassEncoderEnd(passEncoder);
    wgpuRenderPassEncoderRelease(passEncoder);

    // submit to the command buffer
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    WGPUCommandBuffer commandBuffer =
      wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuQueueSubmit(commandQueue, 1, &commandBuffer);

    // present the final frame
    wgpuSurfacePresent(surface);

    // cleanup
    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(targetView);
    wgpuTextureRelease(surfaceTexture.texture);
    wgpuTextureViewRelease(multiSampledTextureView);
    wgpuTextureRelease(multiSampledTexture);
  }

  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  wgpuInstanceRelease(instance);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
