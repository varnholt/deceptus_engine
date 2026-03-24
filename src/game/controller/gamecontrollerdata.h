#pragma once

#include "framework/joystick/gamecontrollerinfo.h"

/// \brief stores the latest sampled controller state shared between systems.
class GameControllerData
{
public:
   /// \brief returns the global controller-data store.
   /// \return singleton instance containing the latest controller snapshot.
   static GameControllerData& getInstance();

   /// \brief returns the current controller information snapshot.
   /// \return immutable reference to cached axis and button values.
   const GameControllerInfo& getJoystickInfo() const;

   /// \brief replaces the cached controller information with a new sample.
   /// \param joystickInfo freshly polled controller state.
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   /// \brief checks whether any controller axis values have been recorded.
   /// \return true when the cached axis-value collection is not empty.
   bool isControllerUsed() const;

private:
   /// \brief constructs an empty controller-data cache.
   GameControllerData() = default;
   GameControllerInfo _joystick_info;
};
