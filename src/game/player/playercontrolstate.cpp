#include "playercontrolstate.h"

#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

namespace {
// global callback for querying playback status - defaults to always returning false
PlayerControlState::PlaybackStatusQuery __playback_status_query = []() { return false; };
} // namespace

bool PlayerControlState::checkState()
{
   // if playback is active, always allow input
   if (__playback_status_query && __playback_status_query()) {
      return true;
   }

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
   // if playback is active, always allow input
   if (__playback_status_query && __playback_status_query()) {
      return true;
   }

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

bool PlayerControlState::checkStateUseInventory()
{
   // this should always be strict, regardless of playback state
   const auto& display_mode = DisplayMode::getInstance();
   if (display_mode.isSet(Display::Modal) || display_mode.isSet(Display::IngameMenu))
   {
      return false;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return false;
   }

   return true;
}

void PlayerControlState::setPlaybackStatusQuery(const PlaybackStatusQuery& query_func)
{
   __playback_status_query = query_func;
}
