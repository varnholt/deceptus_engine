#include "test.h"

// things under test
#include "detonationanimation.h"
#include "framework/math/fbm.h"
#include "framework/math/maptools.h"
#include "framework/math/pathinterpolation.h"

#include <iostream>

void testFbm()
{
   fbm::test();
}

void testBresenham()
{
   auto f = [](int32_t x, int32_t y) -> bool {
      std::cout << x << ", " << y << std::endl;
      return false;
   };

   MapTools::lineCollide(0,0, 10,5, f);
}


void Test::testInterpolation()
{
    PathInterpolation<sf::Vector2f> interpolation;
    std::vector<sf::Vector2f> positions;
    positions.push_back({0.0f, 0.0f});
    positions.push_back({5.0f, 5.0f});
    positions.push_back({0.0f, 0.0f});
    interpolation.addKeys(positions, 5, Easings::Type::EaseInCubic);

    for (const auto& k : interpolation.getTrack())
    {
        std::cout << k._time_value << ": " << k._pos.x << ", " << k._pos.y << std::endl;
    }

    std::cout << Easings::getNameFromEnum<float>(Easings::Type::EaseInCirc) << std::endl;
}

Test::Test()
{
   // testBresenham();
   // testInterpolation();
   // testFbm();
   // exit(0);
}
