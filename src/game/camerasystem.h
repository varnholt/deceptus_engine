#pragma once

#include <SFML/Graphics.hpp>

class CameraSystem
{
   public:

      CameraSystem() = default;

      void update(float viewWidth, float viewHeight);

      float getX() const;
      float getY() const;



   private:

      float mX = 0.0f;
      float mY = 0.0f;

      float mFocusZoneX0 = 0.0f;
      float mFocusZoneX1 = 0.0f;

      float mPanicLineY0 = 0.0f;
      float mPanicLineY1 = 0.0f;

      float mViewWidth = 0.0f;
      float mViewHeight = 0.0f;
};

