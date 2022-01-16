#pragma once

#include <memory>
#include <string>
#include <vector>

#include "json/json.hpp"

#include "extratable.h"
#include "inventory.h"


struct PlayerInfo
{
   Inventory _inventory;
   ExtraTable _extra_table;
   std::string _name;
};


void to_json(nlohmann::json& j, const PlayerInfo& d);
void from_json(const nlohmann::json& j, PlayerInfo& d);
