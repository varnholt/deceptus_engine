#include "maptools.h"

#include <algorithm>
#include <cmath>
#include <utility>

bool MapTools::lineCollide(int32_t x0, int32_t y0, int32_t x1, int32_t y1, std::function<bool(int32_t x, int32_t y)> f)
{
   const auto dx = abs(x1 - x0);
   const auto dy = abs(y1 - y0);
   const auto sx = (x0 < x1) ? 1 : -1;
   const auto sy = (y0 < y1) ? 1 : -1;
   auto err = (dx > dy ? dx : -dy) / 2;
   auto e2 = 0;

   while (true)
   {
      if (f(x0, y0))
      {
         return true;
      }

      if (x0 == x1 && y0 == y1)
      {
         break;
      }

      e2 = err;

      if (e2 > -dx)
      {
         err -= dy;
         x0 += sx;
      }

      if (e2 < dy)
      {
         err += dx;
         y0 += sy;
      }
   }

   return false;
}
