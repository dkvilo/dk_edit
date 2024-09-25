#include "Platform.h"

#if defined(SDL_PLATFORM_MACOS)
#include <Cocoa/Cocoa.h>
#endif

#if defined(SDL_PLATFORM_LINUX)
#include <X11/Xlib.h>
#endif

#if defined(SDL_PLATFORM_WIN32)
#include <windows.h>
#endif

// note (David): extended support for (Linux (x11, wayland), Windows and Macos)
WGPUSurface
SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window)
{
  WGPUSurface surface = nullptr;

#if defined(SDL_PLATFORM_WIN32)
  HWND hwnd = (HWND)SDL_GetPointerProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
  if (hwnd) {
    WGPUChainedStruct nexInChain = {};
    nexInChain.next = NULL;
    nexInChain.sType = WGPUSType_SurfaceDescriptorFromWin32Hwnd;

    WGPUSurfaceDescriptorFromWin32Hwnd fromWin32Hwnd = {};
    fromWin32Hwnd.chain = nexInChain;
    fromWin32Hwnd.hwnd = hwnd;

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.label = NULL;
    surfaceDesc.nextInChain = (const WGPUChainedStruct*)&fromWin32Hwnd;

    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
  }

#elif defined(SDL_PLATFORM_MACOS)
  NSWindow* nswindow = (__bridge NSWindow*)SDL_GetPointerProperty(
    SDL_GetWindowProperties(window),
    SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
    NULL);
  if (nswindow) {
    WGPUChainedStruct nexInChain = {};
    nexInChain.next = NULL;
    nexInChain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;

    WGPUSurfaceDescriptorFromMetalLayer fromMetalLayer = {};
    fromMetalLayer.chain = nexInChain;
    fromMetalLayer.layer =
      (CAMetalLayer*)SDL_GetWindowData(window, "metalLayer");

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.label = NULL;
    surfaceDesc.nextInChain = (const WGPUChainedStruct*)&fromMetalLayer;

    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
  }

#elif defined(SDL_PLATFORM_LINUX)
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
  } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
    struct wl_display* display = (struct wl_display*)SDL_GetPointerProperty(
      SDL_GetWindowProperties(window),
      SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
      NULL);
    struct wl_surface* surfaceWayland =
      (struct wl_surface*)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
        NULL);
    if (display && surfaceWayland) {
      WGPUChainedStruct nexInChain = {};
      nexInChain.next = NULL;
      nexInChain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;

      WGPUSurfaceDescriptorFromWaylandSurface fromWaylandSurface = {};
      fromWaylandSurface.chain = nexInChain;
      fromWaylandSurface.display = display;
      fromWaylandSurface.surface = surfaceWayland;

      WGPUSurfaceDescriptor surfaceDesc = {};
      surfaceDesc.label = NULL;
      surfaceDesc.nextInChain = (const WGPUChainedStruct*)&fromWaylandSurface;

      surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
  }

#elif defined(SDL_PLATFORM_IOS)
  SDL_assert(0);
  SDL_PropertiesID props = SDL_GetWindowProperties(window);
  UIWindow* uiwindow = (__bridge UIWindow*)SDL_GetPointerProperty(
    props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, NULL);
  if (uiwindow) {
    GLuint framebuffer = (GLuint)SDL_GetNumberProperty(
      props, SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER, 0);
    GLuint colorbuffer = (GLuint)SDL_GetNumberProperty(
      props, SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER, 0);
    GLuint resolveFramebuffer = (GLuint)SDL_GetNumberProperty(
      props, SDL_PROP_WINDOW_UIKIT_OPENGL_RESOLVE_FRAMEBUFFER_NUMBER, 0);
  }
#endif

  return surface;
}

// WGPUSurface
// SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window)
// {
//   WGPUSurface surface = nullptr;

//   if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
//     Display* xdisplay =
//       (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
//                                        SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
//                                        NULL);
//     Window xwindow = (Window)SDL_GetNumberProperty(
//       SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
//     if (xdisplay && xwindow) {

//       WGPUChainedStruct nexInChain = {};
//       nexInChain.next = NULL;
//       nexInChain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;

//       WGPUSurfaceDescriptorFromXlibWindow fromXlibWind = {};
//       fromXlibWind.chain = nexInChain;
//       fromXlibWind.display = xdisplay;
//       fromXlibWind.window = xwindow;

//       WGPUSurfaceDescriptor surfaceDesc = {};
//       surfaceDesc.label = NULL;
//       surfaceDesc.nextInChain = (const WGPUChainedStruct*)&fromXlibWind;

//       surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
//     }
//   }

//   return surface;
// }
