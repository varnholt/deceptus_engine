#pragma once

#include <array>
#include <map>
#include <string>

#include "json/json.hpp"

#include "framework/tools/gamepaths.h"
#include "game/player/playerinfo.h"

/// \brief stores one save-slot snapshot, including player info and per-level state.
/// the type also owns static helpers for reading and writing the full three-slot save array.
struct SaveState
{
   /// \brief constructs an empty save slot.
   SaveState() = default;

   /// \brief checks whether this slot has no player profile assigned.
   /// \return true when the player name is empty.
   bool isEmpty() const;

   /// \brief clears player info, level index, and checkpoint to default values.
   void invalidate();

   PlayerInfo _player_info;
   nlohmann::json _level_state;

   int32_t _level_index = 0;
   std::map<std::string, int32_t> _checkpoints;  //!< reached checkpoint index per level description filename

   bool _load_level_requested = false;

   /// \brief returns the checkpoint index reached in one level.
   /// \param level_description_filename path of the level's description json, as listed in levels.json.
   /// \return checkpoint index, or -1 when no checkpoint has been reached in that level yet.
   int32_t getCheckpoint(const std::string& level_description_filename) const;

   /// \brief stores the checkpoint index reached in one level.
   /// \param level_description_filename path of the level's description json, as listed in levels.json.
   /// \param checkpoint_index checkpoint index to remember for that level.
   void setCheckpoint(const std::string& level_description_filename, int32_t checkpoint_index);

   /// \brief returns the checkpoint index reached in the level that is currently loaded.
   /// \return checkpoint index, or -1 when no level is active or no checkpoint has been reached yet.
   static int32_t getCurrentLevelCheckpoint();

   /// \brief stores the checkpoint index reached in the level that is currently loaded.
   /// \param checkpoint_index checkpoint index to remember for the active level.
   static void setCurrentLevelCheckpoint(int32_t checkpoint_index);

   /// \brief computes a user-facing progress percentage for this save slot.
   /// \return integer progress percentage in range 0..100.
   int32_t computeProgress() const;

   /// \brief returns a save slot by index.
   /// \param slot slot index in the internal three-slot array.
   /// \return reference to the selected save slot.
   static SaveState& getSaveState(uint32_t slot);

   /// \brief returns all managed save slots.
   /// \return reference to the static three-slot save array.
   static std::array<SaveState, 3>& getSaveStates();

   /// \brief checks whether all save slots are empty.
   /// \return true when every slot has no player name set.
   static bool allEmpty();

   /// \brief returns player info for the currently selected slot.
   /// \return mutable reference to the current slot player info.
   static PlayerInfo& getPlayerInfo();

   /// \brief returns the currently selected save slot.
   /// \return reference to the active save slot.
   static SaveState& getCurrent();

   /// \brief selects which slot static accessors should operate on.
   /// \param slot slot index to mark as current.
   static void setCurrent(uint32_t slot);

   /// \brief loads save slots from a json file and deserializes into the static array.
   /// \param filename path to the save json file.
   static void deserializeFromFile(const std::string& filename = GamePaths::getPreferencesFile("savestate.json").string());

   /// \brief serializes all save slots and writes them to a json file.
   /// \param filename path to the destination save json file.
   static void serializeToFile(const std::string& filename = GamePaths::getPreferencesFile("savestate.json").string());

   /// \brief patches only the current slot's player stats in an existing save file.
   /// \param filename path to the save json file to update.
   void writePlayerStatsToFile(const std::string& filename = GamePaths::getPreferencesFile("savestate.json").string()) const;

private:
   /// \brief serializes all slots into formatted json text.
   /// \return pretty-printed json string containing the three save slots.
   static std::string serialize();

   /// \brief parses save-state json text into the static save slot array.
   /// \param data json payload representing an array of save slots.
   static void deserialize(const std::string& data);

   static uint32_t __slot;
   static std::array<SaveState, 3> __save_states;
};

/// \brief converts a save slot to json for persistence.
/// \param j destination json object.
/// \param d source save slot.
void to_json(nlohmann::json& j, const SaveState& d);

/// \brief fills a save slot from json fields when present.
/// \param j source json object.
/// \param d destination save slot.
void from_json(const nlohmann::json& j, SaveState& d);
