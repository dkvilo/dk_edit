#include "math.h"
#include <math.h>



bool
IsPointInside(BoundingBox* bbox, Vector2* point)
{
  return (point->x >= bbox->min.x && point->x <= bbox->max.x &&
          point->y >= bbox->min.y && point->y <= bbox->max.y);
}

bool
DoBoundingBoxesIntersect(BoundingBox* a, BoundingBox* b)
{
  if (a->max.x < b->min.x || b->max.x < a->min.x)
    return false;

  if (a->max.y < b->min.y || b->max.y < a->min.y)
    return false;

  return true;
}

BoundingBox
CreateBoundingBoxFromCenter(Vector2* position, float width, float height)
{
  BoundingBox bbox;
  float halfWidth = width / 2.0f;
  float halfHeight = height / 2.0f;

  bbox.min.x = position->x - halfWidth;
  bbox.min.y = position->y - halfHeight;

  bbox.max.x = position->x + halfWidth;
  bbox.max.y = position->y + halfHeight;

  return bbox;
}

BoundingBox
CreateBoundingBoxFromTopLeft(Vector2* position, float width, float height)
{
  BoundingBox bbox;

  bbox.min.x = position->x;
  bbox.min.y = position->y;

  bbox.max.x = position->x + width;
  bbox.max.y = position->y + height;

  return bbox;
}

Quaternion
quaternion(float x, float y, float z, float w)
{
  Quaternion result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

Matrix4
matrix4(float scalar)
{
  Matrix4 result = { 0 };
  for (int i = 0; i < 4 * 4; i++) {
    result.elements[i] = scalar;
  }
  return result;
}

Vector2
vector2(float x, float y)
{
  Vector2 result;
  result.x = x;
  result.y = y;
  return result;
}

Vector3
vector3(float x, float y, float z)
{
  Vector3 result;
  result.x = x;
  result.y = y;
  result.z = z;
  return result;
}

Vector4
vector4(float x, float y, float z, float w)
{
  Vector4 result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

Matrix4
matrix4_identity()
{
  Matrix4 result = { 0 };
  result.elements[0 + 0 * 4] = 1.0f;
  result.elements[1 + 1 * 4] = 1.0f;
  result.elements[2 + 2 * 4] = 1.0f;
  result.elements[3 + 3 * 4] = 1.0f;
  return result;
}

Matrix4
matrix4_orthographic(float left,
                     float right,
                     float bottom,
                     float top,
                     float near,
                     float far)
{
  Matrix4 result = matrix4_identity();
  result.elements[0 + 0 * 4] = 2.0f / (right - left);
  result.elements[1 + 1 * 4] = 2.0f / (top - bottom);
  result.elements[2 + 2 * 4] = 2.0f / (near - far);
  result.elements[0 + 3 * 4] = (left + right) / (left - right);
  result.elements[1 + 3 * 4] = (bottom + top) / (bottom - top);
  result.elements[2 + 3 * 4] = (far + near) / (far - near);
  return result;
}

Matrix4
mat4_perspective(float fov, float aspect_ratio, float near, float far)
{
  Matrix4 result = matrix4_identity();
  float q = 1.0f / tanf(fov * 0.5f);
  result.elements[0 + 0 * 4] = q / aspect_ratio;
  result.elements[1 + 1 * 4] = q;
  result.elements[2 + 2 * 4] = (near + far) / (near - far);
  result.elements[3 + 2 * 4] = -1.0f;
  result.elements[2 + 3 * 4] = (2.0f * near * far) / (near - far);
  return result;
}

Matrix4
matrix4_translate(Matrix4 matrix, Vector3 translation)
{
  Matrix4 result = matrix;
  result.elements[0 + 3 * 4] += translation.x;
  result.elements[1 + 3 * 4] += translation.y;
  result.elements[2 + 3 * 4] += translation.z;
  return result;
}

Matrix4
matrix4_rotate(Matrix4 matrix, float angle, Vector3 axis)
{
  Matrix4 result = matrix;
  float r = angle * (float)MY_DEG_TO_RAD;
  float c = cosf(r);
  float s = sinf(r);
  float omc = 1.0f - c;
  float x = axis.x;
  float y = axis.y;
  float z = axis.z;
  result.elements[0 + 0 * 4] = x * omc + c;
  result.elements[1 + 0 * 4] = y * x * omc + z * s;
  result.elements[2 + 0 * 4] = x * z * omc - y * s;
  result.elements[0 + 1 * 4] = x * y * omc - z * s;
  result.elements[1 + 1 * 4] = y * omc + c;
  result.elements[2 + 1 * 4] = y * z * omc + x * s;
  result.elements[0 + 2 * 4] = x * z * omc + y * s;
  result.elements[1 + 2 * 4] = y * z * omc - x * s;
  result.elements[2 + 2 * 4] = z * omc + c;
  return result;
}

Matrix4
matrix4_scale(Matrix4 matrix, Vector3 scale)
{
  Matrix4 result = matrix;
  result.elements[0 + 0 * 4] *= scale.x;
  result.elements[1 + 1 * 4] *= scale.y;
  result.elements[2 + 2 * 4] *= scale.z;
  return result;
}

Matrix4
matrix4_multiply(Matrix4 left, Matrix4 right)
{
  Matrix4 result = { 0 };
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      float sum = 0.0f;
      for (int e = 0; e < 4; e++) {
        sum += left.elements[e + row * 4] * right.elements[col + e * 4];
      }
      result.elements[col + row * 4] = sum;
    }
  }
  return result;
}

Vector2
vector2_add(Vector2 left, Vector2 right)
{
  Vector2 result;
  result.x = left.x + right.x;
  result.y = left.y + right.y;
  return result;
}

Vector3
vector3_add(Vector3 left, Vector3 right)
{
  Vector3 result;
  result.x = left.x + right.x;
  result.y = left.y + right.y;
  result.z = left.z + right.z;
  return result;
}

Vector4
vector4_add(Vector4 left, Vector4 right)
{
  Vector4 result;
  result.x = left.x + right.x;
  result.y = left.y + right.y;
  result.z = left.z + right.z;
  result.w = left.w + right.w;
  return result;
}

Vector2
vector2_subtract(Vector2 left, Vector2 right)
{
  Vector2 result;
  result.x = left.x - right.x;
  result.y = left.y - right.y;
  return result;
}

Vector3
vector3_subtract(Vector3 left, Vector3 right)
{
  Vector3 result;
  result.x = left.x - right.x;
  result.y = left.y - right.y;
  result.z = left.z - right.z;
  return result;
}

Vector4
vector4_subtract(Vector4 left, Vector4 right)
{
  Vector4 result;
  result.x = left.x - right.x;
  result.y = left.y - right.y;
  result.z = left.z - right.z;
  result.w = left.w - right.w;
  return result;
}

Vector2
vector2_multiply(Vector2 left, Vector2 right)
{
  Vector2 result;
  result.x = left.x * right.x;
  result.y = left.y * right.y;
  return result;
}

Vector3
vector3_multiply(Vector3 left, Vector3 right)
{
  Vector3 result;
  result.x = left.x * right.x;
  result.y = left.y * right.y;
  result.z = left.z * right.z;
  return result;
}

Vector4
vector4_multiply(Vector4 left, Vector4 right)
{
  Vector4 result;
  result.x = left.x * right.x;
  result.y = left.y * right.y;
  result.z = left.z * right.z;
  result.w = left.w * right.w;
  return result;
}

Vector2
vector2_divide(Vector2 left, Vector2 right)
{
  Vector2 result;
  result.x = left.x / right.x;
  result.y = left.y / right.y;
  return result;
}

Vector3
vector3_divide(Vector3 left, Vector3 right)
{
  Vector3 result;
  result.x = left.x / right.x;
  result.y = left.y / right.y;
  result.z = left.z / right.z;
  return result;
}

Vector4
vector4_divide(Vector4 left, Vector4 right)
{
  Vector4 result;
  result.x = left.x / right.x;
  result.y = left.y / right.y;
  result.z = left.z / right.z;
  result.w = left.w / right.w;
  return result;
}

Vector2
vector2_scale(Vector2 vector, float scalar)
{
  Vector2 result;
  result.x = vector.x * scalar;
  result.y = vector.y * scalar;
  return result;
}

Vector3
vector3_scale(Vector3 vector, float scalar)
{
  Vector3 result;
  result.x = vector.x * scalar;
  result.y = vector.y * scalar;
  result.z = vector.z * scalar;
  return result;
}

Vector4
vector4_scale(Vector4 vector, float scalar)
{
  Vector4 result;
  result.x = vector.x * scalar;
  result.y = vector.y * scalar;
  result.z = vector.z * scalar;
  result.w = vector.w * scalar;
  return result;
}

Vector2
vector2_normalize(Vector2 vector)
{
  float length = sqrtf(vector.x * vector.x + vector.y * vector.y);
  Vector2 result;
  result.x = vector.x / length;
  result.y = vector.y / length;
  return result;
}

Vector3
vector3_normalize(Vector3 vector)
{
  float length =
    sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
  Vector3 result;
  result.x = vector.x / length;
  result.y = vector.y / length;
  result.z = vector.z / length;
  return result;
}

Vector4
vector4_normalize(Vector4 vector)
{
  float length = sqrtf(vector.x * vector.x + vector.y * vector.y +
                       vector.z * vector.z + vector.w * vector.w);
  Vector4 result;
  result.x = vector.x / length;
  result.y = vector.y / length;
  result.z = vector.z / length;
  result.w = vector.w / length;
  return result;
}

float
vector2_dot(Vector2 left, Vector2 right)
{
  return left.x * right.x + left.y * right.y;
}

float
vector3_dot(Vector3 left, Vector3 right)
{
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3
vector3_cross(Vector3 left, Vector3 right)
{
  Vector3 result;
  result.x = left.y * right.z - left.z * right.y;
  result.y = left.z * right.x - left.x * right.z;
  result.z = left.x * right.y - left.y * right.x;
  return result;
}

float
vector4_dot(Vector4 left, Vector4 right)
{
  return left.x * right.x + left.y * right.y + left.z * right.z +
         left.w * right.w;
}

Vector4
vector4_cross(Vector4 left, Vector4 right)
{
  Vector4 result;
  result.x = left.y * right.z - left.z * right.y;
  result.y = left.z * right.x - left.x * right.z;
  result.z = left.x * right.y - left.y * right.x;
  result.w = 0.0f;
  return result;
}

Vector2
vector2_rotate(Vector2 vector, float angle)
{
  float r = angle * (float)MY_DEG_TO_RAD;
  float c = cosf(r);
  float s = sinf(r);
  Vector2 result;
  result.x = vector.x * c - vector.y * s;
  result.y = vector.x * s + vector.y * c;
  return result;
}

Vector3
vector3_rotate(Vector3 vector, float angle, Vector3 axis)
{
  float r = angle * (float)MY_DEG_TO_RAD;
  float c = cosf(r);
  float s = sinf(r);
  float omc = 1.0f - c;
  float x = axis.x;
  float y = axis.y;
  float z = axis.z;
  Vector3 result;
  result.x = vector.x * (x * x * omc + c) + vector.y * (y * x * omc + z * s) +
             vector.z * (x * z * omc - y * s);
  result.y = vector.x * (x * y * omc - z * s) + vector.y * (y * y * omc + c) +
             vector.z * (y * z * omc + x * s);
  result.z = vector.x * (x * z * omc + y * s) +
             vector.y * (y * z * omc - x * s) + vector.z * (z * z * omc + c);
  return result;
}

Vector4
vector4_rotate(Vector4 vector, float angle, Vector3 axis)
{
  float r = angle * (float)MY_DEG_TO_RAD;
  float c = cosf(r);
  float s = sinf(r);
  float omc = 1.0f - c;
  float x = axis.x;
  float y = axis.y;
  float z = axis.z;
  Vector4 result;
  result.x = vector.x * (x * x * omc + c) + vector.y * (y * x * omc + z * s) +
             vector.z * (x * z * omc - y * s);
  result.y = vector.x * (x * y * omc - z * s) + vector.y * (y * y * omc + c) +
             vector.z * (y * z * omc + x * s);
  result.z = vector.x * (x * z * omc + y * s) +
             vector.y * (y * z * omc - x * s) + vector.z * (z * z * omc + c);
  result.w = vector.w;
  return result;
}

Vector2
vector2_reflect(Vector2 vector, Vector2 normal)
{
  return vector2_subtract(
    vector, vector2_scale(normal, 2.0f * vector2_dot(vector, normal)));
}

Vector3
vector3_reflect(Vector3 vector, Vector3 normal)
{
  return vector3_subtract(
    vector, vector3_scale(normal, 2.0f * vector3_dot(vector, normal)));
}

Vector4
vector4_reflect(Vector4 vector, Vector4 normal)
{
  return vector4_subtract(
    vector, vector4_scale(normal, 2.0f * vector4_dot(vector, normal)));
}

Vector2
vector2_project(Vector2 vector, Vector2 normal)
{
  return vector2_scale(normal, vector2_dot(vector, normal));
}

Vector3
vector3_project(Vector3 vector, Vector3 normal)
{
  return vector3_scale(normal, vector3_dot(vector, normal));
}

Vector4
vector4_project(Vector4 vector, Vector4 normal)
{
  return vector4_scale(normal, vector4_dot(vector, normal));
}

Vector2
vector2_lerp(Vector2 start, Vector2 end, float t)
{
  return vector2_add(start, vector2_scale(vector2_subtract(end, start), t));
}

Vector3
vector3_lerp(Vector3 start, Vector3 end, float t)
{
  return vector3_add(start, vector3_scale(vector3_subtract(end, start), t));
}

Vector4
vector4_lerp(Vector4 start, Vector4 end, float t)
{
  return vector4_add(start, vector4_scale(vector4_subtract(end, start), t));
}

Vector2
vector2_smoothstep(Vector2 start, Vector2 end, float t)
{
  t = t * t * (3.0f - 2.0f * t);
  return vector2_add(start, vector2_scale(vector2_subtract(end, start), t));
}

Vector3
vector3_smoothstep(Vector3 start, Vector3 end, float t)
{
  t = t * t * (3.0f - 2.0f * t);
  return vector3_add(start, vector3_scale(vector3_subtract(end, start), t));
}

Vector4
vector4_smoothstep(Vector4 start, Vector4 end, float t)
{
  t = t * t * (3.0f - 2.0f * t);
  return vector4_add(start, vector4_scale(vector4_subtract(end, start), t));
}

Vector2
vector2_smootherstep(Vector2 start, Vector2 end, float t)
{
  t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  return vector2_add(start, vector2_scale(vector2_subtract(end, start), t));
}

Vector3
vector3_smootherstep(Vector3 start, Vector3 end, float t)
{
  t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  return vector3_add(start, vector3_scale(vector3_subtract(end, start), t));
}

Vector4
vector4_smootherstep(Vector4 start, Vector4 end, float t)
{
  t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  return vector4_add(start, vector4_scale(vector4_subtract(end, start), t));
}

Vector2
vector2_bezier(Vector2 start, Vector2 control, Vector2 end, float t)
{
  Vector2 a = vector2_lerp(start, control, t);
  Vector2 b = vector2_lerp(control, end, t);
  return vector2_lerp(a, b, t);
}

Vector3
vector3_bezier(Vector3 start, Vector3 control, Vector3 end, float t)
{
  Vector3 a = vector3_lerp(start, control, t);
  Vector3 b = vector3_lerp(control, end, t);
  return vector3_lerp(a, b, t);
}

Vector4
vector4_bezier(Vector4 start, Vector4 control, Vector4 end, float t)
{
  Vector4 a = vector4_lerp(start, control, t);
  Vector4 b = vector4_lerp(control, end, t);
  return vector4_lerp(a, b, t);
}

Vector2
vector2_catmull_rom(Vector2 previous,
                    Vector2 start,
                    Vector2 end,
                    Vector2 next,
                    float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  Vector2 result;
  result.x =
    0.5f * ((2.0f * start.x) + (-previous.x + next.x) * t +
            (2.0f * previous.x - 5.0f * start.x + 4.0f * end.x - next.x) * t2 +
            (-previous.x + 3.0f * start.x - 3.0f * end.x + next.x) * t3);
  result.y =
    0.5f * ((2.0f * start.y) + (-previous.y + next.y) * t +
            (2.0f * previous.y - 5.0f * start.y + 4.0f * end.y - next.y) * t2 +
            (-previous.y + 3.0f * start.y - 3.0f * end.y + next.y) * t3);
  return result;
}

Vector3
vector3_catmull_rom(Vector3 previous,
                    Vector3 start,
                    Vector3 end,
                    Vector3 next,
                    float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  Vector3 result;
  result.x =
    0.5f * ((2.0f * start.x) + (-previous.x + next.x) * t +
            (2.0f * previous.x - 5.0f * start.x + 4.0f * end.x - next.x) * t2 +
            (-previous.x + 3.0f * start.x - 3.0f * end.x + next.x) * t3);
  result.y =
    0.5f * ((2.0f * start.y) + (-previous.y + next.y) * t +
            (2.0f * previous.y - 5.0f * start.y + 4.0f * end.y - next.y) * t2 +
            (-previous.y + 3.0f * start.y - 3.0f * end.y + next.y) * t3);
  result.z =
    0.5f * ((2.0f * start.z) + (-previous.z + next.z) * t +
            (2.0f * previous.z - 5.0f * start.z + 4.0f * end.z - next.z) * t2 +
            (-previous.z + 3.0f * start.z - 3.0f * end.z + next.z) * t3);
  return result;
}

Vector4
vector4_catmull_rom(Vector4 previous,
                    Vector4 start,
                    Vector4 end,
                    Vector4 next,
                    float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  Vector4 result;
  result.x =
    0.5f * ((2.0f * start.x) + (-previous.x + next.x) * t +
            (2.0f * previous.x - 5.0f * start.x + 4.0f * end.x - next.x) * t2 +
            (-previous.x + 3.0f * start.x - 3.0f * end.x + next.x) * t3);
  result.y =
    0.5f * ((2.0f * start.y) + (-previous.y + next.y) * t +
            (2.0f * previous.y - 5.0f * start.y + 4.0f * end.y - next.y) * t2 +
            (-previous.y + 3.0f * start.y - 3.0f * end.y + next.y) * t3);
  result.z =
    0.5f * ((2.0f * start.z) + (-previous.z + next.z) * t +
            (2.0f * previous.z - 5.0f * start.z + 4.0f * end.z - next.z) * t2 +
            (-previous.z + 3.0f * start.z - 3.0f * end.z + next.z) * t3);
  result.w =
    0.5f * ((2.0f * start.w) + (-previous.w + next.w) * t +
            (2.0f * previous.w - 5.0f * start.w + 4.0f * end.w - next.w) * t2 +
            (-previous.w + 3.0f * start.w - 3.0f * end.w + next.w) * t3);
  return result;
}

Vector2
vector2_hermite(Vector2 start,
                Vector2 end,
                Vector2 tangent_start,
                Vector2 tangent_end,
                float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
  float h10 = t3 - 2.0f * t2 + t;
  float h01 = -2.0f * t3 + 3.0f * t2;
  float h11 = t3 - t2;
  Vector2 result;
  result.x =
    h00 * start.x + h10 * tangent_start.x + h01 * end.x + h11 * tangent_end.x;
  result.y =
    h00 * start.y + h10 * tangent_start.y + h01 * end.y + h11 * tangent_end.y;
  return result;
}

Vector3
vector3_hermite(Vector3 start,
                Vector3 end,
                Vector3 tangent_start,
                Vector3 tangent_end,
                float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
  float h10 = t3 - 2.0f * t2 + t;
  float h01 = -2.0f * t3 + 3.0f * t2;
  float h11 = t3 - t2;
  Vector3 result;
  result.x =
    h00 * start.x + h10 * tangent_start.x + h01 * end.x + h11 * tangent_end.x;
  result.y =
    h00 * start.y + h10 * tangent_start.y + h01 * end.y + h11 * tangent_end.y;
  result.z =
    h00 * start.z + h10 * tangent_start.z + h01 * end.z + h11 * tangent_end.z;
  return result;
}

Vector4
vector4_hermite(Vector4 start,
                Vector4 end,
                Vector4 tangent_start,
                Vector4 tangent_end,
                float t)
{
  float t2 = t * t;
  float t3 = t2 * t;
  float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
  float h10 = t3 - 2.0f * t2 + t;
  float h01 = -2.0f * t3 + 3.0f * t2;
  float h11 = t3 - t2;
  Vector4 result;
  result.x =
    h00 * start.x + h10 * tangent_start.x + h01 * end.x + h11 * tangent_end.x;
  result.y =
    h00 * start.y + h10 * tangent_start.y + h01 * end.y + h11 * tangent_end.y;
  result.z =
    h00 * start.z + h10 * tangent_start.z + h01 * end.z + h11 * tangent_end.z;
  result.w =
    h00 * start.w + h10 * tangent_start.w + h01 * end.w + h11 * tangent_end.w;
  return result;
}

Vector2
vector2_zero()
{
  return vector2(0.0f, 0.0f);
}

Vector3
vector3_zero()
{
  return vector3(0.0f, 0.0f, 0.0f);
}

Vector4
vector4_zero()
{
  return vector4(0.0f, 0.0f, 0.0f, 0.0f);
}
