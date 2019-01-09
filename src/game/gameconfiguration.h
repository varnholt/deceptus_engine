#ifndef GAMECONFIGURATION_H
#define GAMECONFIGURATION_H

#include <string>

struct GameConfiguration
{
  GameConfiguration() = default;

  int mVideoModeWidth = 960;
  int mVideoModeHeight = 540;
  int mViewWidth = 480;
  int mViewHeight = 270;
  bool mFullscreen = false;
  float mViewScaleWidth = 1.0f;
  float mViewScaleHeight = 1.0f;

  std::string serialize();
  void deserialize(const std::string& data);

  void deserializeFromFile(const std::string& filename = "data/config/game.json");
  void serializeToFile(const std::string& filename = "data/config/game.json");

  static GameConfiguration sInstance;

  static GameConfiguration& getInstance()
  {
     return sInstance;
  }
};

#endif // GAMECONFIGURATION_H
