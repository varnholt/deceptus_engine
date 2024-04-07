#include "playerstats.h"

using json = nlohmann::json;

void to_json(nlohmann::json& j, const PlayerStats& data)
{
   j = json{{"death_count", data._death_count}};
}

void from_json(const nlohmann::json& j, PlayerStats& data)
{
   if (j.find("death_count") != j.end())
   {
      data._death_count = j.at("death_count").get<int32_t>();
   }
}
