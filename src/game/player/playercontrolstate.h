#ifndef PLAYERCONTROLSTATE_H
#define PLAYERCONTROLSTATE_H

#include <functional>

namespace PlayerControlState
{
///
/// \brief checkState check if the game is in the right state to pass control status information to the player
/// \return true if all is good
///
bool checkState();
bool checkStateCpanOkay();
bool checkStateUseInventory();

// function type for querying playback status
using PlaybackStatusQuery = std::function<bool()>;

// set the function to use for querying playback status
void setPlaybackStatusQuery(const PlaybackStatusQuery& query_func);

};  // namespace PlayerControlState

#endif  // PLAYERCONTROLSTATE_H
