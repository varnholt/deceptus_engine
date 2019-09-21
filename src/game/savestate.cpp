#include "savestate.h"

std::array<SaveState, 3> SaveState::sSaveStates;


std::array<SaveState, 3>& SaveState::getSaveStates()
{
   return sSaveStates;
}

bool SaveState::isEmpty() const
{
   return mEmpty;
}
