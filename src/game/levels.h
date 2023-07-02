#pragma once

#include <string>
#include <vector>

#include "json/json.hpp"

struct LevelItem
{
   std::string _level_name;
};

void from_json(const nlohmann::json& j, LevelItem& item);

namespace Levels
{
LevelItem readLevelItem(int32_t index);
std::vector<LevelItem> readLevelItems();
};  // namespace Levels
