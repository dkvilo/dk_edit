#pragma once

#include "math.h"

struct UIContext
{
  void* activeElement;
  int32_t activeLayer;
  struct {
    Vector2 relative;
    Vector2 global;
    bool pressed;
  } Mouse;
};

struct UIButton
{
  Vector2 position;
  Vector2 size;
  const char* label;
  bool hovered;
  bool clicked;
};

struct UISlider
{
  Vector2 position;
  float width;
  float minValue;
  float maxValue;
  float* value;
  bool hovered;
};

struct UIDropdown
{
  Vector2 position;
  Vector2 size;
  char* options;
  int32_t optionCount;
  int32_t selectedOption;
  bool open;
};
