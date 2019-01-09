#pragma once

#include <string>
#include <vector>

#include "json/json.hpp"
using json = nlohmann::json;


struct LevelItem
{
  std::string mLevelName;
};


void from_json(const json& j, LevelItem& item);


struct Levels
{

  Levels() = default;

  std::vector<LevelItem> mLevels;

  void deserialize(const std::string& data);
  void deserializeFromFile(const std::string& filename = "data/config/levels.json");

  static Levels sInstance;

  static Levels& getInstance()
  {
     return sInstance;
  }

};
