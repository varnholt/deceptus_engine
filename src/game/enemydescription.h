#pragma once

#include "json/json.hpp"

#include "scriptproperty.h"

#include <cstdint>


struct EnemyDescription
{
   EnemyDescription() = default;

   std::string mId;
   std::string mScript;
   std::vector<int32_t> mStartPosition;
   std::vector<int32_t> mPath;
   std::vector<ScriptProperty> mProperties;
   bool mPositionGivenInTiles = true;
   bool mGeneratePatrolPath = false;
};


void to_json(nlohmann::json& j, const EnemyDescription& d);
void from_json(const nlohmann::json& j, EnemyDescription& d);

