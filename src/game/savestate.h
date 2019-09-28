#pragma once

#include <array>
#include <string>

#include "json/json.hpp"

#include "playerinfo.h"

struct SaveState
{
   SaveState() = default;

   bool isEmpty() const;
   void invalidate();

   PlayerInfo mPlayerInfo;

   int32_t mLevelIndex = 0;
   int32_t mCheckpoint = 0;

   int32_t computeProgress() const;

   static SaveState& getSaveState(uint32_t);
   static std::array<SaveState, 3>& getSaveStates();
   static PlayerInfo& getPlayerInfo();

   static SaveState& getCurrent();
   static void setCurrent(uint32_t);

   static void deserializeFromFile(const std::string& filename = "data/config/savestate.json");
   static void serializeToFile(const std::string& filename = "data/config/savestate.json");

private:

   static std::string serialize();
   static void deserialize(const std::string& data);

   static uint32_t sSlot;
   static std::array<SaveState, 3> sSaveStates;
};


void to_json(nlohmann::json& j, const SaveState& d);
void from_json(const nlohmann::json& j, SaveState& d);
