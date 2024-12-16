#include "playerstats.h"

using json = nlohmann::json;

void to_json(nlohmann::json& j, const PlayerStats& data)
{
   j = json{{"death_count_overall", data._death_count_overall}, {"death_count_current_level", data._death_count_current_level}};
}

void from_json(const nlohmann::json& j, PlayerStats& data)
{
   if (j.find("death_count_overall") != j.end())
   {
      data._death_count_overall = j.at("death_count_overall").get<int32_t>();
   }
   if (j.find("death_count_current_level") != j.end())
   {
      data._death_count_current_level = j.at("death_count_current_level").get<int32_t>();
   }
}
