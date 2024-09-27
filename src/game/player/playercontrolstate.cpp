#include "playercontrolstate.h"

#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

bool PlayerControlState::checkState()
{
   const auto& display_mode = DisplayMode::getInstance();
   if (display_mode.isSet(Display::Modal) || display_mode.isSet(Display::CameraPanorama))
   {
      return false;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return false;
   }

   return true;
}

bool PlayerControlState::checkStateCpanOkay()
{
   const auto& display_mode = DisplayMode::getInstance();
   if (display_mode.isSet(Display::Modal))
   {
      return false;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return false;
   }

   return true;
}
