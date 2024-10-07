/**
 * $author David Kviloria <david@skystargames.com>
 * $file main.cpp
 */

#include "pch.h"

#include "Math.h"
#include "backend/2d/Renderer.h"
#include "backend/common.h"

#include "CommandPallete.h"
#include "Editor.h"
#include "Imui.h"
#include "Platform.h"

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720
#define EDITOR_NAME "DKEDIT"

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

  Uint64 lastTime = SDL_GetPerformanceCounter();
  float deltaTime = 0.0f;

  int32_t frameCount = 0;
  float fps = 0.0f;
  float fpsTimer = 0.0f;

  UIContext uiContext = {};

  BatchRenderer batchRenderer(
    device, commandQueue, config.width, config.height);
  batchRenderer.Initialize();

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

  editor.projectConfigPath = "project_config.json";
  editor.loadProjectConfig();

  SDL_StartTextInput(window);

  if (argc > 1) {
    const char* filename = argv[1];
    editor.loadTextFromFile(filename);
    char buffer[255];
    sprintf(buffer, "%s | %s", EDITOR_NAME, filename);
    SDL_SetWindowTitle(window, buffer);
  }

  CommandPalette commandPalette(batchRenderer, config.width, config.height);

  commandPalette.onItemPreview = [&](const CommandPalette::Item& item) {
    switch (commandPalette.getMode()) {
      case CommandPaletteMode::TextSearch:
      case CommandPaletteMode::CommentList:
      case CommandPaletteMode::FunctionList:
        editor.handleCommandPaletteSelection(item.data);
        break;
      case CommandPaletteMode::FileList:
      case CommandPaletteMode::SystemCommand:
        break;
    }
  };

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
      case CommandPaletteMode::TextSearch:
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
      editor.projectConfigPath =
        commandPalette.getWorkDir() + "/project_config.json";
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
            commandPalette.resize(config.width, config.height);
          } break;

          case SDL_EVENT_KEY_DOWN: {
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
