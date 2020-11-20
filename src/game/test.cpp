#include "test.h"

#include "framework/math/maptools.h"

#include <iostream>


void testBresenham()
{
   auto f = [](int32_t x, int32_t y) -> bool {
      std::cout << x << ", " << y << std::endl;
      return false;
   };

   MapTools::lineCollide(0,0, 10,5, f);
}


Test::Test()
{
   // testBresenham();
}
