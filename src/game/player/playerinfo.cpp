#include "playerinfo.h"

using json = nlohmann::json;

void to_json(nlohmann::json& j, const PlayerInfo& data)
{
   j = json{
      {"name", data._name},
      {"inventory", data._inventory},
      {"extras", data._extra_table},
      {"stats", data._extra_table},
      {"weapons", data._weapons}
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
}
