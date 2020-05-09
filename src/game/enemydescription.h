#pragma once

#include "json/json.hpp"

#include "scriptproperty.h"


struct EnemyDescription
{
   EnemyDescription() = default;

   std::string mId;
   std::string mScript;
   std::vector<int> mStartPosition;
   std::vector<int> mPatrolPath;
   std::vector<ScriptProperty> mProperties;
};


void to_json(nlohmann::json& j, const EnemyDescription& d);
void from_json(const nlohmann::json& j, EnemyDescription& d);

