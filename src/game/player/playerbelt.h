#ifndef PLAYERBELT_H
#define PLAYERBELT_H

#include <memory>
#include "playercontrols.h"

/// \brief tracks conveyor belt movement and blends it into horizontal player velocity.
class PlayerBelt
{
public:
   /// \brief adjusts desired horizontal velocity to account for active belt movement and movement input.
   /// \param desired_velocity in-out desired horizontal velocity in meters per second.
   /// \param max_velocity maximum allowed player speed used when amplifying movement with the belt.
   /// \param controls current player controls used to detect movement direction.
   void applyBeltVelocity(float& desired_velocity, float max_velocity, const std::shared_ptr<PlayerControls>& controls);

   /// \brief gets the configured conveyor belt velocity.
   /// \return belt velocity contribution in meters per second.
   float getBeltVelocity() const;
   /// \brief stores the current conveyor belt velocity.
   /// \param belt_velocity belt velocity contribution in meters per second.
   void setBeltVelocity(float belt_velocity);

   /// \brief reports whether the player is currently standing on a belt surface.
   /// \return true when belt velocity should affect movement.
   bool isOnBelt() const;
   /// \brief marks whether the player is currently on a belt.
   /// \param on_belt true when belt logic should be active.
   void setOnBelt(bool on_belt);

private:
   float _belt_velocity = 0.0f;
   bool _is_on_belt = false;
};

#endif  // PLAYERBELT_H
