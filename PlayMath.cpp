#include "PlayMath.h"
#include <math.h>

void
DrawCartesianAxes(BatchRenderer& renderer,
                  const CartesianCoordinateSystem& system)
{
  Vector2 xAxisStart = { 0, system.origin.y }; // for X-axis
  Vector2 xAxisEnd = { system.screenSize.x, system.origin.y };
  renderer.AddLine(
    xAxisStart, xAxisEnd, 4.0f, GREEN, 0.0f, ORIGIN_CENTER, LAYER_UI);

  Vector2 yAxisStart = { system.origin.x, 0 }; // for Y-axis
  Vector2 yAxisEnd = { system.origin.x, system.screenSize.y };
  renderer.AddLine(
    yAxisStart, yAxisEnd, 4.0f, BLUE, 0.0f, ORIGIN_CENTER, LAYER_UI);
}

void
DrawCartesianGrid(BatchRenderer& renderer,
                  const CartesianCoordinateSystem& system)
{
  for (float x = system.origin.x; x <= system.screenSize.x; x += system.scale) {
    Vector2 lineStart = { x, 0 };
    Vector2 lineEnd = { x, system.screenSize.y };
    renderer.AddLine(lineStart,
                     lineEnd,
                     1.0f,
                     system.gridColor,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);
  }

  for (float x = system.origin.x; x >= 0; x -= system.scale) {
    Vector2 lineStart = { x, 0 };
    Vector2 lineEnd = { x, system.screenSize.y };
    renderer.AddLine(lineStart,
                     lineEnd,
                     1.0f,
                     system.gridColor,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);
  }

  for (float y = system.origin.y; y <= system.screenSize.y; y += system.scale) {
    Vector2 lineStart = { 0, system.screenSize.y - y };
    Vector2 lineEnd = { system.screenSize.x, system.screenSize.y - y };
    renderer.AddLine(lineStart,
                     lineEnd,
                     1.0f,
                     system.gridColor,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);
  }

  for (float y = system.origin.y; y >= 0; y -= system.scale) {
    Vector2 lineStart = { 0, system.screenSize.y - y };
    Vector2 lineEnd = { system.screenSize.x, system.screenSize.y - y };
    renderer.AddLine(lineStart,
                     lineEnd,
                     1.0f,
                     system.gridColor,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);
  }
}

void
DrawCartesianLabels(BatchRenderer& renderer,
                    const CartesianCoordinateSystem& system)
{
  for (float x = system.origin.x; x <= system.screenSize.x; x += system.scale) {
    char buffer[16];
    sprintf(buffer, "%.0f", (x - system.origin.x) / system.scale);
    Vector2 labelPos = { x, system.screenSize.y - (system.origin.y - 5) };
    renderer.DrawText(buffer, labelPos, 20.0f, system.textColor, LAYER_UI);
  }

  for (float x = system.origin.x; x >= 0; x -= system.scale) {
    char buffer[16];
    sprintf(buffer, "%.0f", (x - system.origin.x) / system.scale);
    Vector2 labelPos = { x, system.screenSize.y - (system.origin.y - 5) };
    renderer.DrawText(buffer, labelPos, 20.0f, system.textColor, LAYER_UI);
  }

  for (float y = system.origin.y; y <= system.screenSize.y; y += system.scale) {
    char buffer[16];
    sprintf(buffer, "%.0f", (y - system.origin.y) / system.scale);
    Vector2 labelPos = { system.origin.x + 5, system.screenSize.y - y };
    renderer.DrawText(buffer, labelPos, 20.0f, system.textColor, LAYER_UI);
  }

  for (float y = system.origin.y; y >= 0; y -= system.scale) {
    char buffer[16];
    sprintf(buffer, "%.0f", (y - system.origin.y) / system.scale);
    Vector2 labelPos = { system.origin.x + 5, system.screenSize.y - y };
    renderer.DrawText(buffer, labelPos, 20.0f, system.textColor, LAYER_UI);
  }
}

void
Visualize2DVector(BatchRenderer& renderer,
                  Vector2 start,
                  Vector2 vector,
                  float thickness,
                  Vector4 color)
{
  Vector2 end = { start.x + vector.x, start.y + vector.y };
  renderer.AddLine(start, end, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);

  float arrowLength = 10.0f;
  float arrowAngle = M_PI / 6.0f;

  float angle = atan2(vector.y, vector.x);
  Vector2 arrowLeft = { end.x - arrowLength * cos(angle - arrowAngle),
                        end.y - arrowLength * sin(angle - arrowAngle) };
  Vector2 arrowRight = { end.x - arrowLength * cos(angle + arrowAngle),
                         end.y - arrowLength * sin(angle + arrowAngle) };

  renderer.AddLine(
    end, arrowLeft, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
  renderer.AddLine(
    end, arrowRight, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
}

void
Visualize2DVectorWithInfo(BatchRenderer& renderer,
                          Vector2 start,
                          Vector2 vector,
                          float thickness,
                          Vector4 color,
                          Vector4 textColor)
{
  Vector2 end = { start.x + vector.x, start.y + vector.y };

  renderer.AddLine(start, end, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);

  float arrowLength = 10.0f;
  float arrowAngle = M_PI / 6.0f;

  float angleRadians = atan2(vector.y, vector.x);
  float angleDegrees = angleRadians * (180.0f / M_PI);

  Vector2 arrowLeft = { end.x - arrowLength * cos(angleRadians - arrowAngle),
                        end.y - arrowLength * sin(angleRadians - arrowAngle) };
  Vector2 arrowRight = { end.x - arrowLength * cos(angleRadians + arrowAngle),
                         end.y - arrowLength * sin(angleRadians + arrowAngle) };

  renderer.AddLine(
    end, arrowLeft, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
  renderer.AddLine(
    end, arrowRight, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);

  float length = sqrt(vector.x * vector.x +
                      vector.y * vector.y); // todo (David): add to math lib

  char angleText[64];
  char lengthText[64];
  sprintf(angleText, "Angle: %.2f deg", angleDegrees);
  sprintf(lengthText, "Length: %.2f units", length);

  Vector2 midPoint = { (start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f };

  Vector2 angleTextPos = { midPoint.x + 15.0f, midPoint.y - 15.0f };
  Vector2 lengthTextPos = { midPoint.x + 15.0f, midPoint.y + 15.0f };

  renderer.DrawText(angleText, angleTextPos, 20.0f, textColor, LAYER_UI);
  renderer.DrawText(lengthText, lengthTextPos, 20.0f, textColor, LAYER_UI);
}

void
VisualizePoint(BatchRenderer& renderer,
               Vector2 point,
               float size,
               Vector4 color)
{
  renderer.AddQuad({ point.x - size / 2, point.y - size / 2 },
                   size,
                   size,
                   color,
                   0.0f,
                   ORIGIN_CENTER,
                   LAYER_UI);
}

void
VisualizePointWithInfo(BatchRenderer& renderer,
                       Vector2 point,
                       float size,
                       Vector4 pointColor,
                       Vector4 textColor)
{
  VisualizePoint(renderer, point, size, pointColor);

  char pointText[64];
  sprintf(pointText, "(%.2f, %.2f)", point.x, point.y);
  Vector2 textPos = { point.x + size / 2 + 5.0f, point.y - size / 2 - 20.0f };
  renderer.DrawText(pointText, textPos, 20.0f, textColor, LAYER_UI);
}

Vector2
QuadraticBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, float t)
{
  float oneMinusT = 1.0f - t;
  return {
    oneMinusT * oneMinusT * p0.x + 2 * oneMinusT * t * p1.x + t * t * p2.x,
    oneMinusT * oneMinusT * p0.y + 2 * oneMinusT * t * p1.y + t * t * p2.y
  };
}

Vector2
CubicBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t)
{
  float oneMinusT = 1.0f - t;
  return { oneMinusT * oneMinusT * oneMinusT * p0.x +
             3 * oneMinusT * oneMinusT * t * p1.x +
             3 * oneMinusT * t * t * p2.x + t * t * t * p3.x,

           oneMinusT * oneMinusT * oneMinusT * p0.y +
             3 * oneMinusT * oneMinusT * t * p1.y +
             3 * oneMinusT * t * t * p2.y + t * t * t * p3.y };
}

void
DrawQuadraticBezier(BatchRenderer& renderer,
                    Vector2 p0,
                    Vector2 p1,
                    Vector2 p2,
                    Vector4 color,
                    float thickness)
{
  const int segments = 100;
  Vector2 previousPoint = p0;

  for (int i = 1; i <= segments; ++i) {
    float t = (float)i / (float)segments;
    Vector2 currentPoint = QuadraticBezierPoint(p0, p1, p2, t);

    renderer.AddLine(previousPoint,
                     currentPoint,
                     thickness,
                     color,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);

    previousPoint = currentPoint;
  }
}

void
DrawCubicBezier(BatchRenderer& renderer,
                Vector2 p0,
                Vector2 p1,
                Vector2 p2,
                Vector2 p3,
                Vector4 color,
                float thickness)
{
  const int segments = 100;
  Vector2 previousPoint = p0;

  for (int i = 1; i <= segments; ++i) {
    float t = (float)i / (float)segments;
    Vector2 currentPoint = CubicBezierPoint(p0, p1, p2, p3, t);
    renderer.AddLine(previousPoint,
                     currentPoint,
                     thickness,
                     color,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);

    previousPoint = currentPoint;
  }
}

void
HandleMouseDragging(ControlPoint& controlPoint,
                    Vector2 mousePos,
                    bool mousePressed)
{
  if (controlPoint.isDragged && mousePressed) {
    controlPoint.position = mousePos;
  }

  if (IsMouseOverPoint(mousePos, controlPoint.position, 10.0f) &&
      mousePressed) {
    controlPoint.isDragged = true;
  }

  if (!mousePressed) {
    controlPoint.isDragged = false;
  }
}

void
DrawControlPoint(BatchRenderer& renderer,
                 ControlPoint& controlPoint,
                 float size,
                 Vector4 color)
{
  renderer.AddQuad(
    { controlPoint.position.x - size / 2, controlPoint.position.y - size / 2 },
    size,
    size,
    color,
    0.0f,
    ORIGIN_CENTER,
    LAYER_UI);
}

void
DrawQuadraticBezierControlHandles(BatchRenderer& renderer,
                                  Vector2 p0,
                                  Vector2 p1,
                                  Vector2 p2,
                                  Vector4 color,
                                  float thickness)
{
  renderer.AddLine(p0, p1, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
  renderer.AddLine(p1, p2, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
}

void
DrawCubicBezierControlHandles(BatchRenderer& renderer,
                              Vector2 p0,
                              Vector2 p1,
                              Vector2 p2,
                              Vector2 p3,
                              Vector4 color,
                              float thickness)
{
  renderer.AddLine(p0, p1, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
  renderer.AddLine(p1, p2, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
  renderer.AddLine(p2, p3, thickness, color, 0.0f, ORIGIN_CENTER, LAYER_UI);
}

void
DrawSineWave(BatchRenderer& renderer,
             Vector2 origin,
             float amplitude,
             float frequency,
             float phase,
             float length,
             Vector4 color,
             float thickness)
{
  const int segments = 100;
  float step = length / (float)segments;

  Vector2 previousPoint = { origin.x, origin.y + amplitude * sin(phase) };

  for (int i = 1; i <= segments; ++i) {
    float x = origin.x + i * step;
    float t = i * step; // dist along X axis
    float y = origin.y + amplitude * sin(frequency * t + phase);

    Vector2 currentPoint = { x, y };
    renderer.AddLine(previousPoint,
                     currentPoint,
                     thickness,
                     color,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);

    previousPoint = currentPoint;
  }
}

void
DrawCosineWave(BatchRenderer& renderer,
               Vector2 origin,
               float amplitude,
               float frequency,
               float phase,
               float length,
               Vector4 color,
               float thickness)
{
  const int segments = 100;
  float step = length / (float)segments;

  Vector2 previousPoint = { origin.x, origin.y + amplitude * cos(phase) };

  for (int i = 1; i <= segments; ++i) {
    float x = origin.x + i * step;
    float t = i * step;
    float y = origin.y + amplitude * cos(frequency * t + phase);

    Vector2 currentPoint = { x, y };
    renderer.AddLine(previousPoint,
                     currentPoint,
                     thickness,
                     color,
                     0.0f,
                     ORIGIN_CENTER,
                     LAYER_UI);

    previousPoint = currentPoint;
  }
}
