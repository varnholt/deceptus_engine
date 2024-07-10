#pragma once

#include <memory>
#include <string>
#include <vector>

#include "json/json.hpp"

#include "game/player/extratable.h"
#include "game/player/inventory.h"
#include "game/player/playerstats.h"
#include "game/player/weaponsystem.h"

struct PlayerInfo
{
   Inventory _inventory;
   ExtraTable _extra_table;
   PlayerStats _stats;
   WeaponSystem _weapons;

   std::string _name;
};


void to_json(nlohmann::json& j, const PlayerInfo& d);
void from_json(const nlohmann::json& j, PlayerInfo& d);
