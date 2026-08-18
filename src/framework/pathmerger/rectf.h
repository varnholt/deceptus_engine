#pragma once

namespace PathMerge
{

/// \brief An axis-aligned bounding rectangle stored as (x1,y1)-(x2,y2).
struct RectF
{
   double x1 = 0.0;  //!< Left edge.
   double y1 = 0.0;  //!< Top edge.
   double x2 = 0.0;  //!< Right edge.
   double y2 = 0.0;  //!< Bottom edge.

   [[nodiscard]] constexpr bool intersects(const RectF& other) const
   {
      return x1 <= other.x2 && x2 >= other.x1 && y1 <= other.y2 && y2 >= other.y1;
   }

   [[nodiscard]] constexpr double minCoord(int axis) const
   {
      return axis == 0 ? x1 : y1;
   }
   [[nodiscard]] constexpr double maxCoord(int axis) const
   {
      return axis == 0 ? x2 : y2;
   }
   constexpr double& minCoordRef(int axis)
   {
      return axis == 0 ? x1 : y1;
   }
   constexpr double& maxCoordRef(int axis)
   {
      return axis == 0 ? x2 : y2;
   }
};

}  // namespace PathMerge
