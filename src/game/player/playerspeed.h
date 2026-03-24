#ifndef PLAYERSPEED_H
#define PLAYERSPEED_H

#include "box2d/box2d.h"

/// \brief stores horizontal movement tuning and the current velocity vector.
struct PlayerSpeed
{
   /// \brief creates a speed state with zero current velocity.
   PlayerSpeed()
   {
      _current_velocity.SetZero();
   }

   /// \brief creates a speed state with explicit runtime values.
   /// \param current_velocity initial velocity vector used by movement code.
   /// \param velocity_max maximum allowed speed.
   /// \param acceleration acceleration rate while input is applied.
   /// \param deceleration deceleration rate while input is released or reversed.
   PlayerSpeed(const b2Vec2& current_velocity, float velocity_max, float acceleration, float deceleration)
       : _current_velocity(current_velocity), _velocity_max(velocity_max), _acceleration(acceleration), _deceleration(deceleration)
   {
   }

   b2Vec2 _current_velocity;
   float _velocity_max = 0.0f;
   float _acceleration = 0.0f;
   float _deceleration = 0.0f;
};

#endif  // PLAYERSPEED_H
