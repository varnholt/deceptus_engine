#pragma once

#include <cstdint>
#include <functional>


class MapTools
{
public:
   static bool lineCollide(
      int32_t x0,
      int32_t y0,
      int32_t x1,
      int32_t y1,
      std::function<bool(int32_t x, int32_t y)> f
   );
};

