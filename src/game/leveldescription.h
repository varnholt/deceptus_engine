#pragma once

#include "json/json.hpp"

#include "enemydescription.h"


struct LevelDescription
{
   LevelDescription() = default;

   std::string mFilename;
   std::vector<int> mStartPosition;
   std::vector<EnemyDescription> mEnemies;

   static std::shared_ptr<LevelDescription> load(const std::string& path);
};


void to_json(nlohmann::json& j, const LevelDescription& d);
void from_json(const nlohmann::json& j, LevelDescription& d);

