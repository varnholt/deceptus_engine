#pragma once

namespace fbm
{

///
/// \brief Minimal 2D vector helper used by fbm noise functions.
///
struct vec2
{
   float x = 0.0f;
   float y = 0.0f;

   ///
   /// \brief Returns component-wise sum with another vector.
   /// \param v Right-hand vector.
   /// \return Summed vector.
   ///
   vec2 operator+(vec2& v)
   {
      return vec2{x + v.x, y + v.y};
   }

   ///
   /// \brief Returns component-wise sum with another vector.
   /// \param v Right-hand vector.
   /// \return Summed vector.
   ///
   vec2 operator+(vec2 v)
   {
      return vec2{x + v.x, y + v.y};
   }

   ///
   /// \brief Returns component-wise difference to another vector.
   /// \param v Right-hand vector.
   /// \return Difference vector.
   ///
   vec2 operator-(vec2& v)
   {
      return vec2{x - v.x, y - v.y};
   }

   ///
   /// \brief Returns component-wise difference to another vector.
   /// \param v Right-hand vector.
   /// \return Difference vector.
   ///
   vec2 operator-(vec2 v)
   {
      return vec2{x - v.x, y - v.y};
   }

   ///
   /// \brief Adds scalar to both components.
   /// \param s Scalar value.
   /// \return Result vector.
   ///
   vec2 operator+(float s)
   {
      return vec2{x + s, y + s};
   }

   ///
   /// \brief Subtracts scalar from both components.
   /// \param s Scalar value.
   /// \return Result vector.
   ///
   vec2 operator-(float s)
   {
      return vec2{x - s, y - s};
   }

   ///
   /// \brief Multiplies both components by scalar.
   /// \param s Scalar value.
   /// \return Result vector.
   ///
   vec2 operator*(float s)
   {
      return vec2{x * s, y * s};
   }

   ///
   /// \brief Divides both components by scalar.
   /// \param s Scalar value.
   /// \return Result vector.
   ///
   vec2 operator/(float s)
   {
      return vec2{x / s, y / s};
   }

   ///
   /// \brief Adds another vector in place.
   /// \param v Right-hand vector.
   /// \return Updated vector reference.
   ///
   vec2& operator+=(vec2& v)
   {
      x += v.x;
      y += v.y;
      return *this;
   }

   ///
   /// \brief Subtracts another vector in place.
   /// \param v Right-hand vector.
   /// \return Updated vector reference.
   ///
   vec2& operator-=(vec2& v)
   {
      x -= v.x;
      y -= v.y;
      return *this;
   }

   ///
   /// \brief Multiplies both components in place by scalar.
   /// \param s Scalar value.
   /// \return Updated vector reference.
   ///
   vec2& operator*=(float s)
   {
      x *= s;
      y *= s;
      return *this;
   }

   ///
   /// \brief Multiplies two vectors component-wise.
   /// \param b Right-hand vector.
   /// \return Component-wise product.
   ///
   vec2 operator*(vec2 b)
   {
      return vec2{x * b.x, y * b.y};
   }

   ///
   /// \brief Divides two vectors component-wise.
   /// \param b Right-hand vector.
   /// \return Component-wise quotient.
   ///
   vec2 operator/(vec2& b)
   {
      return vec2{x / b.x, y / b.y};
   }
};

///
/// \brief Returns 2d dot product of two vectors.
/// \param v1 First vector.
/// \param v2 Second vector.
/// \return Dot product value.
///
float dot(vec2 v1, vec2 v2);

///
/// \brief Returns fractional part of a floating-point value.
/// \param x Input value.
/// \return Fractional component in [0,1).
///
float fract(float x);

///
/// \brief Linearly interpolates between `a` and `b`.
/// \param a Start value.
/// \param b End value.
/// \param x Blend factor.
/// \return Interpolated value.
///
float mix(float a, float b, float x);

///
/// \brief Pseudo-random value from a 2d coordinate using sine hashing.
/// \param st Input coordinate.
/// \return Deterministic noise value in [0,1).
///
float random1(const vec2& st);

///
/// \brief Value noise using `random1` on a unit grid with smooth interpolation.
/// \param st Input coordinate.
/// \return Smooth noise value.
///
float noise1(const vec2& st);

///
/// \brief One-dimensional hash used by the second noise variant.
/// \param n Input scalar.
/// \return Deterministic pseudo-random value in [0,1).
///
float hash(float n);

///
/// \brief Two-dimensional hash used by the second noise variant.
/// \param p Input coordinate.
/// \return Deterministic pseudo-random value in [0,1).
///
float hash(const vec2& p);

///
/// \brief Value noise using `hash` on a unit grid with smooth interpolation.
/// \param st Input coordinate.
/// \return Smooth noise value.
///
float noise2(const vec2& st);

///
/// \brief Fractal brownian motion built from multiple octaves of `noise2`.
/// \param st Input coordinate.
/// \return Combined fbm value.
///
float fbm(vec2 st);

///
/// \brief Prints a 1d sample strip of fbm values for quick inspection.
///
void test();

}  // namespace fbm
