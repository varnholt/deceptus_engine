#pragma once

#include "json/json.hpp"

#include "enemydescription.h"


struct LevelDescription
{
   LevelDescription() = default;

   std::string _filename;
   std::vector<int32_t> _start_position;
   std::vector<EnemyDescription> _enemies;

   static std::shared_ptr<LevelDescription> load(const std::string& path);
};


void to_json(nlohmann::json& j, const LevelDescription& d);
void from_json(const nlohmann::json& j, LevelDescription& d);

