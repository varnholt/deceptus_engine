#pragma once

#include "json/json.hpp"

struct LevelState
{
   public:
      LevelState();
};


void to_json(nlohmann::json& j, const LevelState& d);
void from_json(const nlohmann::json& j, LevelState& d);
