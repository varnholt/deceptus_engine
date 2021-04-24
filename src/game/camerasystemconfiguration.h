#pragma once

#include <string>


class CameraSystemConfiguration
{
   public:

      static CameraSystemConfiguration& getInstance();

      float getCameraVelocityFactorX() const;
      float getFocusZoneDivider() const;
      float getTargetShiftFactor() const;
      int32_t getBackInBoundsToleranceX() const;

      float getCameraVelocityFactorY() const;
      float getPanicLineDivider() const;
      float getViewRatioY() const;
      int32_t getBackInBoundsToleranceY() const;
      int32_t getPlayerOffsetY() const;


   private:

      std::string serialize();
      void deserialize(const std::string& data);
      void deserializeFromFile(const std::string& filename = "data/config/camera.json");
      void serializeToFile(const std::string& filename = "data/config/camera.json");

      // x
      float mCameraVelocityFactorX = 4.0f;
      float mFocusZoneDivider = 6.0f;
      float mTargetShiftFactor = 0.75f;
      int32_t mBackInBoundsToleranceX = 10;
      float mRoomDampingFactorX = 64.0f;

      // y
      float mCameraVelocityFactorY = 3.0f;
      float mPanicLineDivider = 2.5f;
      float mViewRatioY = 1.5f;
      int32_t mBackInBoundsToleranceY = 10;
      int32_t mPlayerOffsetY = 0;
      float mRoomDampingFactorY = 32.0f;

      static bool sInitialized;
      static CameraSystemConfiguration sInstance;
};


