#pragma once

#include <algorithm>
#include <cmath>

#include "pointf.h"

namespace PathMerge
{

/// \brief Internal math helpers shared across the path-simplification algorithm.
namespace MathHelpers
{

[[nodiscard]] inline bool fuzzyIsNull(double value)
{
   return std::abs(value) <= 1e-12;
}

[[nodiscard]] inline bool fuzzyCompare(double first, double second)
{
   return std::abs(first - second) * 1000000000000.0 <= std::min(std::abs(first), std::abs(second));
}

[[nodiscard]] inline bool comparePoints(const PointF& point_a, const PointF& point_b)
{
   if (fuzzyCompare(point_a.x, point_b.x))
   {
      return point_a.y < point_b.y;
   }
   return point_a.x < point_b.x;
}

[[nodiscard]] constexpr double dot(const PointF& vector_a, const PointF& vector_b)
{
   return vector_a.x * vector_b.x + vector_a.y * vector_b.y;
}

[[nodiscard]] inline PointF normalize(const PointF& vector)
{
   const double length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
   if (length < 1e-12)
   {
      return {0.0, 0.0};
   }
   return {vector.x / length, vector.y / length};
}

[[nodiscard]] constexpr double coordByAxis(const PointF& point, int axis)
{
   return axis == 0 ? point.x : point.y;
}

}  // namespace MathHelpers

}  // namespace PathMerge
