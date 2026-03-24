#ifndef PLAYERCONTROLSTATE_H
#define PLAYERCONTROLSTATE_H

#include <functional>

namespace PlayerControlState
{
/// \brief determines whether regular player controls should be processed.
/// \return true when the game runs and no blocking modal or camera panorama is active, or when event playback is active.
bool checkState();

/// \brief determines whether controls that allow camera-panorama mode should be processed.
/// \return true when controls are allowed while only modal overlays are blocked.
bool checkStateCpanOkay();

/// \brief determines whether inventory slots can be used right now.
/// \return true when the game is running and neither modal dialogs nor the ingame menu are open.
bool checkStateUseInventory();

// function type for querying playback status
using PlaybackStatusQuery = std::function<bool()>;

// set the function to use for querying playback status
/// \brief installs the callback used to detect whether input playback is active.
/// \param query_func callback that returns true while replayed input is running.
void setPlaybackStatusQuery(const PlaybackStatusQuery& query_func);

};  // namespace PlayerControlState

#endif  // PLAYERCONTROLSTATE_H
