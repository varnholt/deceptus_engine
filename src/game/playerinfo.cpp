#include "playerinfo.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const PlayerInfo& data)
{
   j = json{
      {"name", data.mName},
      {"inventory", data.mInventory}
   };
}


void from_json(const nlohmann::json& j, PlayerInfo& data)
{
   data.mName = j.at("name").get<std::string>();
   data.mInventory = j.at("inventory").get<Inventory>();
}


