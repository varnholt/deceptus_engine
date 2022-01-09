#pragma once

#include <array>
#include <string>

#include "json/json.hpp"

#include "player/playerinfo.h"


/*! \brief SaveState is the data written to disk when needed.
 *         That includes for instance reached checkpoints and collected extras.
 *
 * SaveStates are written to disk each time the player reaches a checkpoint.
 */
struct SaveState
{
   SaveState() = default;

   bool isEmpty() const;
   void invalidate();

   PlayerInfo _player_info;

   uint32_t _level_index = 0;
   uint32_t _checkpoint = 0;

   bool _load_level_requested = false;

   int32_t computeProgress() const;

   static SaveState& getSaveState(uint32_t);
   static std::array<SaveState, 3>& getSaveStates();
   static bool allEmpty();
   static PlayerInfo& getPlayerInfo();

   static SaveState& getCurrent();
   static void setCurrent(uint32_t);

   static void deserializeFromFile(const std::string& filename = "data/config/savestate.json");
   static void serializeToFile(const std::string& filename = "data/config/savestate.json");

private:

   static std::string serialize();
   static void deserialize(const std::string& data);

   static uint32_t __slot;
   static std::array<SaveState, 3> __save_states;
};


void to_json(nlohmann::json& j, const SaveState& d);
void from_json(const nlohmann::json& j, SaveState& d);
