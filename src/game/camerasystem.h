#pragma once

#include <SFML/Graphics.hpp>

class CameraSystem
{
   public:

      void update(float viewWidth, float viewHeight);

      float getX() const;
      float getY() const;

      float getFocusZoneX0() const;
      float getFocusZoneX1() const;

      float getPanicLineY0() const;
      float getPanicLineY1() const;


      static CameraSystem& getCameraSystem();


   private:

      CameraSystem() = default;

      float mX = 0.0f;
      float mY = 0.0f;

      float mFocusZoneX0 = 0.0f;
      float mFocusZoneX1 = 0.0f;

      float mPanicLineY0 = 0.0f;
      float mPanicLineY1 = 0.0f;

      float mViewWidth = 0.0f;
      float mViewHeight = 0.0f;

      static CameraSystem sInstance;
};

