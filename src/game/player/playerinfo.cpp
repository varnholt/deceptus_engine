#include "playerinfo.h"

#include "game/player/inventoryconfig.h"

using json = nlohmann::json;

PlayerInfo::PlayerInfo()
{
   _inventory_config.linkInventoryToItemSystem(_inventory, _items);
   AchievementDefinitions::loadDefinitions("data/config/achievements.json");
   TreasureDefinitions::loadDefinitions("data/config/treasures.json");
}

void to_json(nlohmann::json& j, const PlayerInfo& data)
{
   j = json{
      {"name", data._name},
      {"inventory", data._inventory},
      {"extras", data._extra_table},
      {"stats", data._stats},
      {"weapons", data._weapons},
      {"achievements", data._achievements},
      {"treasures", data._treasures},
   };
}

void from_json(const nlohmann::json& j, PlayerInfo& data)
{
   data._name = j.at("name").get<std::string>();

   if (j.find("inventory") != j.end())
   {
      data._inventory = j.at("inventory").get<Inventory>();
   }

   if (j.find("extras") != j.end())
   {
      data._extra_table = j.at("extras").get<ExtraTable>();
   }

   if (j.find("stats") != j.end())
   {
      data._stats = j.at("stats").get<PlayerStats>();
   }

   if (j.find("weapons") != j.end())
   {
      data._weapons = j.at("weapons").get<WeaponSystem>();
   }

   if (j.find("achievements") != j.end())
   {
      data._achievements = j.at("achievements").get<Achievements>();
   }

   if (j.find("treasures") != j.end())
   {
      data._treasures = j.at("treasures").get<Treasures>();
   }

   // re-link inventory callbacks after deserialization
   data._inventory_config.linkInventoryToItemSystem(data._inventory, data._items);
}
