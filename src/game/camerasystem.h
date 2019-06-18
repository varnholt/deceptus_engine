#pragma once

#include <SFML/Graphics.hpp>

class CameraSystem
{
   public:

      CameraSystem() = default;

      void update();


   private:

      int32_t mX = 0;
      int32_t mY = 0;

      int32_t mFocusZoneX0 = 0;
      int32_t mFocusZoneX1 = 0;

      int32_t mPanicLineY0 = 0;
      int32_t mPanicLineY1 = 0;
};

