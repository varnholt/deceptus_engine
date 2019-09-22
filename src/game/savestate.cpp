#include "savestate.h"

std::array<SaveState, 3> SaveState::sSaveStates;
uint32_t SaveState::sSlot = 0;

std::array<SaveState, 3>& SaveState::getSaveStates()
{
   return sSaveStates;
}

bool SaveState::isEmpty() const
{
   return mEmpty;
}


PlayerInfo& SaveState::getPlayerInfo()
{
   return sSaveStates[sSlot].mPlayerInfo;
}
