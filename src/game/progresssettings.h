#pragma once

#include <string>

class ProgressSettings
{

public:

  ProgressSettings() = default;

  std::string mLevel;
  int mCheckpoint = 0;

  std::string serialize();
  void deserialize(const std::string& data);

  void deserializeFromFile(const std::string& filename = "data/config/progress.json");
  void serializeToFile(const std::string& filename = "data/config/progress.json");


  static ProgressSettings sInstance;

  static ProgressSettings& getInstance()
  {
     return sInstance;
  }

};
