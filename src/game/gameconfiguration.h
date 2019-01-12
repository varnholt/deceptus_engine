#ifndef GAMECONFIGURATION_H
#define GAMECONFIGURATION_H

#include <cstdint>
#include <string>

struct GameConfiguration
{
  GameConfiguration() = default;

  int32_t mVideoModeWidth = 960;
  int32_t mVideoModeHeight = 540;
  int32_t mViewWidth = 480;
  int32_t mViewHeight = 270;
  bool mFullscreen = false;
  float mViewScaleWidth = 1.0f;
  float mViewScaleHeight = 1.0f;

  int32_t mAudioVolumeMaster = 50;
  int32_t mAudioVolumeSfx = 50;
  int32_t mAudioVolumeMusic = 50;

  void deserializeFromFile(const std::string& filename = "data/config/game.json");
  void serializeToFile(const std::string& filename = "data/config/game.json");

  static bool sInitialized;
  static GameConfiguration sInstance;

  static GameConfiguration& getInstance()
  {
     if (!sInitialized)
     {
         sInstance.deserializeFromFile();
         sInitialized = true;
     }

     return sInstance;
  }

private:
  std::string serialize();
  void deserialize(const std::string& data);
};

#endif // GAMECONFIGURATION_H
