#pragma once

#include <array>
#include <string>

#include "json/json.hpp"

#include "playerinfo.h"

struct SaveState
{

   SaveState() = default;

   bool isEmpty() const;

   PlayerInfo mPlayerInfo;

   bool mEmpty = true;
   int32_t mLevelIndex = 0;
   int32_t mCheckpoint = 0;

   static void deserialize(const std::string& data);
   static void deserializeFromFile(const std::string& filename = "data/config/savestate.json");

   static std::string serialize();
   static void serializeToFile(const std::string& filename = "data/config/savestate.json");


   static uint32_t sSlot;
   static std::array<SaveState, 3>& getSaveStates();
   static PlayerInfo& getPlayerInfo();
   static std::array<SaveState, 3> sSaveStates;
};


void to_json(nlohmann::json& j, const SaveState& d);
void from_json(const nlohmann::json& j, SaveState& d);
