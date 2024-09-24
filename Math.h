/**
 * $author David Kviloria <david@skystargames.com>
 * $file math.cpp
 */
#if !defined(__MATH_H__)
#define __MATH_H__

#include <stdint.h>

typedef struct Vector2
{
  float x;
  float y;
} Vector2;

typedef struct Vector3
{
  float x;
  float y;
  float z;
} Vector3;

typedef struct Vector4
{
  float x;
  float y;
  float z;
  float w;
} Vector4;

typedef struct Matrix4
{
  float elements[16];
} Matrix4;

typedef struct Quaternion
{
  float x;
  float y;
  float z;
  float w;
} Quaternion;

typedef struct BoundingBox
{
  Vector2 min;
  Vector2 max;
} BoundingBox;

#define MY_PI 3.14159265358979323846
#define MY_HALF_PI 1.57079632679489661923
#define MY_TWO_PI 6.28318530717958647692
#define MY_EPSILON 0.00001f
#define MY_DEG_TO_RAD (MY_PI / 180.0f)
#define MY_RAD_TO_DEG (180.0f / MY_PI)

bool
DoBoundingBoxesIntersect(BoundingBox* a, BoundingBox* b);

bool
IsPointInside(BoundingBox* bbox, Vector2* point);

BoundingBox
CreateBoundingBoxFromCenter(Vector2* position, float width, float height);

BoundingBox
CreateBoundingBoxFromTopLeft(Vector2* position, float width, float height);

bool
IsMouseOverPoint(Vector2 mousePos, Vector2 pointPos, float radius);

float
Clamp(float value, float minVal, float maxVal);

Quaternion
quaternion(float x, float y, float z, float w);

Vector2
vector2(float x, float y);

Vector3
vector3(float x, float y, float z);

Vector4
vector4(float x, float y, float z, float w);

Matrix4
matrix4(float scalar);

Matrix4
matrix4_identity();

Matrix4
matrix4_orthographic(float left,
                     float right,
                     float bottom,
                     float top,
                     float near,
                     float far);

Matrix4
matrix4_perspective(float fov, float aspect_ratio, float near, float far);

Matrix4
matrix4_translate(Matrix4 matrix, Vector3 translation);

Matrix4
matrix4_rotate(Matrix4 matrix, float angle, Vector3 axis);

Matrix4
matrix4_scale(Matrix4 matrix, Vector3 scale);

Matrix4
matrix4_multiply(Matrix4 left, Matrix4 right);

Vector2
vector2_add(Vector2 left, Vector2 right);

Vector3
vector3_add(Vector3 left, Vector3 right);

Vector4
vector4_add(Vector4 left, Vector4 right);

Vector2
vector2_subtract(Vector2 left, Vector2 right);

Vector3
vector3_subtract(Vector3 left, Vector3 right);

Vector4
vector4_subtract(Vector4 left, Vector4 right);

Vector2
vector2_multiply(Vector2 left, Vector2 right);

Vector3
vector3_multiply(Vector3 left, Vector3 right);

Vector4
vector4_multiply(Vector4 left, Vector4 right);

Vector2
vector2_divide(Vector2 left, Vector2 right);

Vector3
vector3_divide(Vector3 left, Vector3 right);

Vector4
vector4_divide(Vector4 left, Vector4 right);

Vector2
vector2_scale(Vector2 vector, float scalar);

Vector3
vector3_scale(Vector3 vector, float scalar);

Vector4
vector4_scale(Vector4 vector, float scalar);

Vector2
vector2_normalize(Vector2 vector);

Vector3
vector3_normalize(Vector3 vector);

Vector4
vector4_normalize(Vector4 vector);

float
vector2_dot(Vector2 left, Vector2 right);

float
vector3_dot(Vector3 left, Vector3 right);

float
vector4_dot(Vector4 left, Vector4 right);

Vector3
vector3_cross(Vector3 left, Vector3 right);

Vector4
vector4_cross(Vector4 left, Vector4 right);

Vector2
vector2_lerp(Vector2 start, Vector2 end, float t);

float
vector2_distance(Vector2 a, Vector2 b);

float
lerp(float a, float b, float t);

Vector3
vector3_lerp(Vector3 start, Vector3 end, float t);

Vector4
vector4_lerp(Vector4 start, Vector4 end, float t);

Vector2
vector2_smoothstep(Vector2 start, Vector2 end, float t);

Vector3
vector3_smoothstep(Vector3 start, Vector3 end, float t);

Vector4
vector4_smoothstep(Vector4 start, Vector4 end, float t);

Vector2
vector2_smootherstep(Vector2 start, Vector2 end, float t);

Vector3
vector3_smootherstep(Vector3 start, Vector3 end, float t);

Vector4
vector4_smootherstep(Vector4 start, Vector4 end, float t);

Vector2
vector2_bezier(Vector2 start, Vector2 control, Vector2 end, float t);

Vector3
vector3_bezier(Vector3 start, Vector3 control, Vector3 end, float t);

Vector4
vector4_bezier(Vector4 start, Vector4 control, Vector4 end, float t);

Vector2
vector2_catmull_rom(Vector2 previous,
                    Vector2 start,
                    Vector2 end,
                    Vector2 next,
                    float t);

Vector3
vector3_catmull_rom(Vector3 previous,
                    Vector3 start,
                    Vector3 end,
                    Vector3 next,
                    float t);

Vector4
vector4_catmull_rom(Vector4 previous,
                    Vector4 start,
                    Vector4 end,
                    Vector4 next,
                    float t);

Vector2
vector2_hermite(Vector2 start,
                Vector2 end,
                Vector2 tangent_start,
                Vector2 tangent_end,
                float t);

Vector3
vector3_hermite(Vector3 start,
                Vector3 end,
                Vector3 tangent_start,
                Vector3 tangent_end,
                float t);

Vector4
vector4_hermite(Vector4 start,
                Vector4 end,
                Vector4 tangent_start,
                Vector4 tangent_end,
                float t);

Vector2
vector2_rotate(Vector2 vector, float angle);

Vector3
vector3_rotate(Vector3 vector, float angle, Vector3 axis);

Vector4
vector4_rotate(Vector4 vector, float angle, Vector3 axis);

Vector2
vector2_reflect(Vector2 vector, Vector2 normal);

Vector3
vector3_reflect(Vector3 vector, Vector3 normal);

Vector4
vector4_reflect(Vector4 vector, Vector4 normal);

Vector2
vector2_project(Vector2 vector, Vector2 normal);

Vector3
vector3_project(Vector3 vector, Vector3 normal);

Vector4
vector4_project(Vector4 vector, Vector4 normal);

Vector2
vector2_reject(Vector2 vector, Vector2 normal);

Vector3
vector3_reject(Vector3 vector, Vector3 normal);

Vector4
vector4_reject(Vector4 vector, Vector4 normal);

Vector2
vector2_zero();

Vector3
vector3_zero();

Vector4
vector4_zero();

#endif // __MATH_H__