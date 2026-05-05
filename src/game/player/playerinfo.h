#pragma once

#include <memory>
#include <string>
#include <vector>

#include "json/json.hpp"

#include "game/player/achievements.h"
#include "game/player/extratable.h"
#include "game/player/inventory.h"
#include "game/player/inventoryconfig.h"
#include "game/player/itemsystem.h"
#include "game/player/playerstats.h"
#include "game/player/treasures.h"
#include "game/player/weaponsystem.h"

/// \brief persistent player profile data used for save and load operations.
struct PlayerInfo
{
   /// \brief initializes player profile data and links inventory behavior to the item system.
   PlayerInfo();

   InventoryConfig _inventory_config;
   Inventory _inventory;
   ExtraTable _extra_table;
   PlayerStats _stats;
   WeaponSystem _weapons;
   ItemSystem _items;
   Achievements _achievements;
   Treasures _treasures;

   std::string _name;
};

/// \brief serializes player profile fields to json.
/// \param j destination json object.
/// \param d player profile data to serialize.
void to_json(nlohmann::json& j, const PlayerInfo& d);

/// \brief deserializes player profile fields from json and re-links inventory item callbacks.
/// \param j source json object.
/// \param d player profile instance that receives deserialized values.
void from_json(const nlohmann::json& j, PlayerInfo& d);
