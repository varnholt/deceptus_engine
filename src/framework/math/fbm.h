#pragma once

namespace fbm
{

struct vec2
{
   float x = 0.0f;
   float y = 0.0f;

   vec2 operator+(vec2& v)
   {
      return vec2{x + v.x, y + v.y};
   }
   vec2 operator+(vec2 v)
   {
      return vec2{x + v.x, y + v.y};
   }
   vec2 operator-(vec2& v)
   {
      return vec2{x - v.x, y - v.y};
   }
   vec2 operator-(vec2 v)
   {
      return vec2{x - v.x, y - v.y};
   }
   vec2 operator+(float s)
   {
      return vec2{x + s, y + s};
   }
   vec2 operator-(float s)
   {
      return vec2{x - s, y - s};
   }
   vec2 operator*(float s)
   {
      return vec2{x * s, y * s};
   }
   vec2 operator/(float s)
   {
      return vec2{x / s, y / s};
   }
   vec2& operator+=(vec2& v)
   {
      x += v.x;
      y += v.y;
      return *this;
   }
   vec2& operator-=(vec2& v)
   {
      x -= v.x;
      y -= v.y;
      return *this;
   }
   vec2& operator*=(float s)
   {
      x *= s;
      y *= s;
      return *this;
   }
   vec2 operator*(vec2 b)
   {
      return vec2{x * b.x, y * b.y};
   }
   vec2 operator/(vec2& b)
   {
      return vec2{x / b.x, y / b.y};
   }
};

float dot(vec2 v1, vec2 v2);

float fract(float x);
float mix(float a, float b, float x);

// random version 1
float random1(const vec2& st);
float noise1(const vec2& st);

// random version 2
float hash(float n);
float hash(const vec2& p);
float noise2(const vec2& st);

float fbm(vec2 st);
void test();

}  // namespace fbm
