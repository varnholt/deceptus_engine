#pragma once

#include <cstdint>
#include <string>

class ProgressSettings
{

public:

  ProgressSettings() = default;

  std::string _level;
  int32_t _checkpoint = 0;

  std::string serialize();
  void deserialize(const std::string& data);

  void deserializeFromFile(const std::string& filename = "data/config/progress.json");
  void serializeToFile(const std::string& filename = "data/config/progress.json");

  static ProgressSettings& getInstance();

};
