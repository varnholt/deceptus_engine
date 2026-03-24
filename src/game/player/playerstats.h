#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <cstdint>

#include "json/json.hpp"

/// \brief stores persistent death counters for profile and level tracking.
struct PlayerStats
{
   int32_t _death_count_overall{0};
   int32_t _death_count_current_level{0};  // if player goes back and forth between levels, this should be adjusted
};

/// \brief serializes player death counters into json fields.
/// \param j json object that receives "death_count_overall" and "death_count_current_level".
/// \param d player stats source data.
void to_json(nlohmann::json& j, const PlayerStats& d);

/// \brief deserializes available death counters from json into player stats.
/// \param j json object that may contain death counter fields.
/// \param d player stats target updated for fields present in json.
void from_json(const nlohmann::json& j, PlayerStats& d);

#endif  // PLAYERSTATS_H
