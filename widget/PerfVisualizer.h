#pragma once

#include "../backend/2d/Renderer.h"

class PerfVisualizer
{
public:
  PerfVisualizer(BatchRenderer& renderer,
                Vector2 origin,
                float width,
                float height,
                int maxSegments)
    : renderer(renderer)
    , origin(origin)
    , width(width)
    , height(height)
    , maxSegments(maxSegments)
    , dynamicMaxFPS(120.0f)
  {
    fpsHistory.reserve(maxSegments);
  }

  void AddFPS(float fps)
  {
    float clampedFPS = Clamp(fps, 0.0f, 120.0f);
    bool isDrop = false;

    if (!fpsHistory.empty() && clampedFPS < fpsHistory.back().fps) {
      isDrop = true;
    }

    fpsHistory.emplace_back(FPSFrame{ clampedFPS, isDrop });

    if (clampedFPS > dynamicMaxFPS) {
      dynamicMaxFPS = clampedFPS;
    }

    // maintain the history
    if (fpsHistory.size() > maxSegments) {
      // if the removed FPS was the dynamic max, recalculate dynamicMaxFPS
      if (fpsHistory.front().fps >= dynamicMaxFPS) {
        dynamicMaxFPS = clampedFPS;
        for (const auto& frame : fpsHistory) {
          if (frame.fps > dynamicMaxFPS) {
            dynamicMaxFPS = frame.fps;
          }
        }
      }
      fpsHistory.erase(fpsHistory.begin());
    }
  }

  void Draw(const Vector4& lineColor, const Vector4& spikeColor)
  {
    if (fpsHistory.empty())
      return;

    float segmentWidth = width / (float)(maxSegments - 1);

    float bar_width = 10.0f;
    float bar_height = 50.0f;
    float pad = 5.0f;

    for (size_t i = 0; i < fpsHistory.size(); ++i) {
      float x = origin.x + i * segmentWidth;
      float padding = pad * i;
      Vector2 position = { x + padding, origin.y };

      Vector4 color = fpsHistory[i].isDrop ? spikeColor : lineColor;
      renderer.AddQuad(
        position, bar_width, bar_height, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
    }
  }

private:
  struct FPSFrame
  {
    float fps;
    bool isDrop;
  };

  BatchRenderer& renderer;
  Vector2 origin;
  float width;
  float height;
  int maxSegments;
  float dynamicMaxFPS;
  std::vector<FPSFrame> fpsHistory;
};
