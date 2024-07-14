#include "playercontrolstate.h"

#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

bool PlayerControlState::checkState()
{
   if (DisplayMode::getInstance().isSet(Display::Modal))
   {
      return false;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return false;
   }

   return true;
}
