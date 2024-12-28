#ifndef PLAYERCONTROLSTATE_H
#define PLAYERCONTROLSTATE_H

namespace PlayerControlState
{
///
/// \brief checkState check if the game is in the right state to pass control status information to the player
/// \return true if all is good
///
bool checkState();
bool checkStateCpanOkay();
bool checkStateUseInventory();

};  // namespace PlayerControlState

#endif  // PLAYERCONTROLSTATE_H
