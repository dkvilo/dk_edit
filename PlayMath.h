/**
 * $author David Kviloria <david@skystargames.com>
 * Cartesian system related library for plotting different graphs.
 */

#pragma once

#include "Math.h"
#include "backend/2d/Renderer.h"

struct ControlPoint
{
  Vector2 position;
  bool isDragged;
};

struct CartesianCoordinateSystem
{
  Vector2 origin;
  Vector2 screenSize;
  // scale of the grid (e.g., pixels per unit)
  float scale;  
  Vector4 axisColor;
  Vector4 gridColor;
  Vector4 textColor;
};

void
DrawCartesianAxes(BatchRenderer& renderer,
                  const CartesianCoordinateSystem& system);

void
DrawCartesianGrid(BatchRenderer& renderer,
                  const CartesianCoordinateSystem& system);

void
DrawCartesianLabels(BatchRenderer& renderer,
                    const CartesianCoordinateSystem& system);
void
Visualize2DVector(BatchRenderer& renderer,
                  Vector2 start,
                  Vector2 vector,
                  float thickness,
                  Vector4 color);

void
Visualize2DVectorWithInfo(BatchRenderer& renderer,
                          Vector2 start,
                          Vector2 vector,
                          float thickness,
                          Vector4 color,
                          Vector4 textColor);

void
VisualizePoint(BatchRenderer& renderer,
               Vector2 point,
               float size,
               Vector4 color);

void
VisualizePointWithInfo(BatchRenderer& renderer,
                       Vector2 point,
                       float size,
                       Vector4 pointColor,
                       Vector4 textColor);

Vector2
QuadraticBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, float t);

Vector2
CubicBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t);

void
DrawQuadraticBezier(BatchRenderer& renderer,
                    Vector2 p0,
                    Vector2 p1,
                    Vector2 p2,
                    Vector4 color,
                    float thickness);

void
DrawCubicBezier(BatchRenderer& renderer,
                Vector2 p0,
                Vector2 p1,
                Vector2 p2,
                Vector2 p3,
                Vector4 color,
                float thickness);

void
HandleMouseDragging(ControlPoint& controlPoint,
                    Vector2 mousePos,
                    bool mousePressed);

void
DrawControlPoint(BatchRenderer& renderer,
                 ControlPoint& controlPoint,
                 float size,
                 Vector4 color);

void
DrawQuadraticBezierControlHandles(BatchRenderer& renderer,
                                  Vector2 p0,
                                  Vector2 p1,
                                  Vector2 p2,
                                  Vector4 color,
                                  float thickness);

void
DrawCubicBezierControlHandles(BatchRenderer& renderer,
                              Vector2 p0,
                              Vector2 p1,
                              Vector2 p2,
                              Vector2 p3,
                              Vector4 color,
                              float thickness);

void
DrawSineWave(BatchRenderer& renderer,
             Vector2 origin,
             float amplitude,
             float frequency,
             float phase,
             float length,
             Vector4 color,
             float thickness);

void
DrawCosineWave(BatchRenderer& renderer,
               Vector2 origin,
               float amplitude,
               float frequency,
               float phase,
               float length,
               Vector4 color,
               float thickness);