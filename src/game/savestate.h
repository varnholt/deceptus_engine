#pragma once

#include <array>
#include <string>

#include "playerinfo.h"

class SaveState
{

public:

   SaveState() = default;

   static std::array<SaveState, 3>& getSaveStates();
   bool isEmpty() const;

   static PlayerInfo& getPlayerInfo();


private:

   PlayerInfo mPlayerInfo;
   bool mEmpty = true;
   int32_t mLevelIndex = 0;
   int32_t mCheckpoint = 0;

   static uint32_t sSlot;
   static std::array<SaveState, 3> sSaveStates;
};

