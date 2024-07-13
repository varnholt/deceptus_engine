#include "test.h"

// things under test
#include "framework/math/fbm.h"
#include "framework/math/maptools.h"
#include "framework/math/pathinterpolation.h"
#include "game/animation/detonationanimation.h"
#include "game/animation/framemapper.h"

#include <iostream>

void testFbm()
{
   fbm::test();
}

void testBresenham()
{
   auto f = [](int32_t x, int32_t y) -> bool
   {
      std::cout << x << ", " << y << std::endl;
      return false;
   };

   MapTools::lineCollide(0, 0, 10, 5, f);
}

void testFrameMapper()
{
   FrameMapper<int32_t> frame_times{{50, 50, 50, 120, 3000}, 0};

   auto index = 0;
   for (auto val : frame_times._frame_durations_accumulated)
   {
      std::cout << index << ": " << val << std::endl;
      index++;
   }

   // 0: 50 -> 0..50
   // 1: 50 -> 51..100
   // 2: 50 -> 101..151
   // 3: 120 -> 151..270
   // 4: 3000 -> 270..3000

   // 0: 50
   // 1: 100
   // 2: 150
   // 3: 270
   // 4: 3270

   // 0 at 0
   // 20 at 0
   // 100 at 1
   // 500 at 3
   // 10000 at 4

   std::vector<int32_t> search_values{0, 20, 100, 500, 10000};
   for (auto search_value : search_values)
   {
      std::cout << search_value << " at " << frame_times[search_value] << std::endl;
   }
}

void Test::testInterpolation()
{
   PathInterpolation<sf::Vector2f> interpolation;
   std::vector<sf::Vector2f> positions;
   positions.emplace_back(0.0f, 0.0f);
   positions.emplace_back(5.0f, 5.0f);
   positions.emplace_back(0.0f, 0.0f);
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
   // testFrameMapper();
   // exit(0);
}
