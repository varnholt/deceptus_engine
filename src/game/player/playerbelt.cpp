#include "playerbelt.h"


void PlayerBelt::applyBeltVelocity(float& desired_velocity, float max_velocity, const std::shared_ptr<PlayerControls>& controls)
{
   if (!isOnBelt())
   {
      return;
   }

   if (getBeltVelocity() < 0.0f)
   {
      if (controls->isMovingRight())
      {
         desired_velocity *= 0.5f;
      }
      else if (controls->isMovingLeft())
      {
         if (desired_velocity > 0.0f)
         {
            desired_velocity = 0.0f;
         }

         desired_velocity *= 2.0f;
         desired_velocity = std::min(desired_velocity, max_velocity);
      }
      else
      {
         desired_velocity += getBeltVelocity();
      }
   }
   else if (getBeltVelocity() > 0.0f)
   {
      if (controls->isMovingLeft())
      {
         desired_velocity *= 0.5f;
      }
      else if (controls->isMovingRight())
      {
         if (desired_velocity < 0.0f)
         {
            desired_velocity = 0.0f;
         }

         desired_velocity *= 2.0f;
         desired_velocity = std::max(desired_velocity, -max_velocity);
      }
      else
      {
         desired_velocity += getBeltVelocity();
      }
   }
}

float PlayerBelt::getBeltVelocity() const
{
   return _belt_velocity;
}

void PlayerBelt::setBeltVelocity(float belt_velocity)
{
   _belt_velocity = belt_velocity;
}

bool PlayerBelt::isOnBelt() const
{
   return _is_on_belt;
}

void PlayerBelt::setOnBelt(bool on_belt)
{
   _is_on_belt = on_belt;
}
