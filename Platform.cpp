#include "Platform.h"
#include <X11/Xlib.h>

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
