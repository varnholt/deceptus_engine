#include "test.h"

// things under test
#include "framework/math/maptools.h"
#include "detonationanimation.h"

#include <iostream>


void testBresenham()
{
   auto f = [](int32_t x, int32_t y) -> bool {
      std::cout << x << ", " << y << std::endl;
      return false;
   };

   MapTools::lineCollide(0,0, 10,5, f);
}


void testDetonationAnimation()
{
   DetonationAnimation::unitTest1();
}


Test::Test()
{
   // testBresenham();

   testDetonationAnimation();
}
