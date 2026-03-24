#pragma once

namespace fbm
{

///
/// \brief Represents vec2 state and behavior.
///

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

///

/// \brief Executes dot.

/// \param v1 V1.

/// \param v2 V2.

/// \return Requested value.

///

float dot(vec2 v1, vec2 v2);

///

/// \brief Executes fract.

/// \param x Horizontal coordinate.

/// \return Requested value.

///

float fract(float x);
///
/// \brief Executes mix.
/// \param a A.
/// \param b B.
/// \param x Horizontal coordinate.
/// \return Requested value.
///
float mix(float a, float b, float x);

///

/// \brief Executes random1.

/// \param st St.

/// \return Requested value.

///

// random version 1
float random1(const vec2& st);
///
/// \brief Executes noise1.
/// \param st St.
/// \return Requested value.
///
float noise1(const vec2& st);

///

/// \brief Executes hash.

/// \param n N.

/// \return Requested value.

///

// random version 2
float hash(float n);
///
/// \brief Executes hash.
/// \param p P.
/// \return Requested value.
///
float hash(const vec2& p);
///
/// \brief Executes noise2.
/// \param st St.
/// \return Requested value.
///
float noise2(const vec2& st);

///

/// \brief Executes fbm.

/// \param st St.

/// \return Requested value.

///

float fbm(vec2 st);
///
/// \brief Executes test.
///
void test();

}  // namespace fbm
