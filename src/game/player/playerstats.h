#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <cstdint>

#include "json/json.hpp"

struct PlayerStats
{
   int32_t _death_count_overall{0};
   int32_t _death_count_current_level{0};  // if player goes back and forth between levels, this should be adjusted
};

void to_json(nlohmann::json& j, const PlayerStats& d);
void from_json(const nlohmann::json& j, PlayerStats& d);

#endif  // PLAYERSTATS_H
