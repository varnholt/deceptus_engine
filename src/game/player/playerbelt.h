#ifndef PLAYERBELT_H
#define PLAYERBELT_H

#include <memory>
#include "playercontrols.h"

class PlayerBelt
{
public:
   void applyBeltVelocity(float& desired_velocity, float max_velocity, const std::shared_ptr<PlayerControls>& controls);

   float getBeltVelocity() const;
   void setBeltVelocity(float belt_velocity);

   bool isOnBelt() const;
   void setOnBelt(bool on_belt);

private:
   float _belt_velocity = 0.0f;
   bool _is_on_belt = false;
};

#endif  // PLAYERBELT_H
