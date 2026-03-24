#pragma once

#include <cstdint>
#include <functional>

///
/// \brief contains discrete grid-collision helpers for map queries.
///
class MapTools
{
public:
   ///
   /// \brief walks a grid line from (x0,y0) to (x1,y1) and tests each cell.
   /// \param x0 start x coordinate.
   /// \param y0 start y coordinate.
   /// \param x1 end x coordinate.
   /// \param y1 end y coordinate.
   /// \param f predicate called for each visited cell.
   /// \return `true` when predicate returns true for any visited cell.
   ///
   static bool lineCollide(int32_t x0, int32_t y0, int32_t x1, int32_t y1, std::function<bool(int32_t x, int32_t y)> f);
};
