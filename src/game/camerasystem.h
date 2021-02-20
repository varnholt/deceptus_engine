#pragma once

#include "camerasystemconfiguration.h"
#include "room.h"

#include <SFML/Graphics.hpp>

#include <optional>

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

      void setRoom(const std::optional<Room>& room);

      void syncNow();

      static CameraSystem& getCameraSystem();


   private:

      CameraSystem() = default;

      void updateX();
      void updateY();

      float mX = 0.0f;
      float mY = 0.0f;

      float mFocusZoneX0 = 0.0f;
      float mFocusZoneX1 = 0.0f;
      float mFocusZoneCenter = 0.0f;
      float mFocusOffset = 0.0f;

      float mPanicLineY0 = 0.0f;
      float mPanicLineY1 = 0.0f;

      float mViewWidth = 0.0f;
      float mViewHeight = 0.0f;

      bool mFocusXTriggered = false;
      bool mFocusYTriggered = false;

      std::optional<Room> mRoom;
      float mRoomInterpolation = 0.0f;
      float mRoomX = 0.0f;
      float mRoomY = 0.0f;

      static CameraSystem sInstance;
};

