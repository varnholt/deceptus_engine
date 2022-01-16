#include "playerinfo.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const PlayerInfo& data)
{
   j = json{
      {"name", data._name},
      {"inventory", data._inventory},
      {"extras", data._extra_table}
   };
}


void from_json(const nlohmann::json& j, PlayerInfo& data)
{
   data._name = j.at("name").get<std::string>();
   data._inventory = j.at("inventory").get<Inventory>();
   data._extra_table = j.at("extras").get<ExtraTable>();
}


