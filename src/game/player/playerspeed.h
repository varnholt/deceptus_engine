#ifndef PLAYERSPEED_H
#define PLAYERSPEED_H

#include <box2d/box2d.h>

struct PlayerSpeed
{
   PlayerSpeed()
   {
      _current_velocity.SetZero();
   }

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
