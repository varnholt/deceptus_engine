#pragma once

#include <cstdint>
#include <string>

struct GameConfiguration
{
   int32_t mVideoModeWidth = 1280;
   int32_t mVideoModeHeight = 720;
   int32_t mViewWidth = 640;
   int32_t mViewHeight = 360;
   bool mFullscreen = false;
   float mViewScaleWidth = 1.0f;
   float mViewScaleHeight = 1.0f;
   float mBrightness = 0.5f;
   bool mVSync = false;

   int32_t mAudioVolumeMaster = 50;
   int32_t mAudioVolumeSfx = 50;
   int32_t mAudioVolumeMusic = 50;

   void deserializeFromFile(const std::string& filename = "data/config/game.json");
   void serializeToFile(const std::string& filename = "data/config/game.json");

   static GameConfiguration& getDefaults();
   static GameConfiguration& getInstance();

   static void resetAudioDefaults();

private:

   static bool sInitialized;

   static GameConfiguration sInstance;
   static GameConfiguration mDefaults;

   std::string serialize();
   void deserialize(const std::string& data);

   GameConfiguration() = default;
};

