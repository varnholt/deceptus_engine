#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <cstdint>

#include "json/json.hpp"

struct PlayerStats
{
   int32_t _death_count{0};
};

void to_json(nlohmann::json& j, const PlayerStats& d);
void from_json(const nlohmann::json& j, PlayerStats& d);

#endif  // PLAYERSTATS_H
