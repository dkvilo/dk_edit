#pragma once

#include "math.h"

struct UIContext
{
  void* activeElement;
  int32_t activeLayer;
  struct
  {
    Vector2 relative;
    Vector2 global;
    bool pressed;
  } Mouse;
};
