#include "playerinfo.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const PlayerInfo& data)
{
   j = json{
      {"name", data.mName}
   };
}


void from_json(const nlohmann::json& j, PlayerInfo& data)
{
   data.mName = j.at("name").get<std::string>();
}


