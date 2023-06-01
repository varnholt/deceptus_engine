#include "playerdash.h"

#include "Box2D/Box2D.h"
#include "audio.h"
#include "physics/physicsconfiguration.h"
#include "savestate.h"

void PlayerDash::update(const DashInput& input)
{
   if (!(SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(Skill::SkillType::Dash)))
   {
      return;
   }

   if (input._wallsliding)
   {
      // abort dash when wallslide becomes active
      if (hasMoreFrames())
      {
         // abort dash
         abort();

         // reset dash
         reset(input.player_body);
         return;
      }

      return;
   }

   if (input._hard_landing)
   {
      return;
   }

   // don't allow a new dash move inside water
   if (input._is_in_water)
   {
      if (!hasMoreFrames())
      {
         reset(input.player_body);
         return;
      }
   }

   auto keepLinearVelocity = [input]()
   {
      auto velocity = input.player_body->GetLinearVelocity();
      velocity.y = 0.0f;
      input.player_body->SetLinearVelocity(velocity);
   };

   auto dir = input._dir;

   // dir is the initial dir passed in on button press
   // Dash::None is passed in on regular updates after the initial press
   if (dir != Dash::None)
   {
      // prevent dash spam 1: dash is still active
      if (hasMoreFrames())
      {
         return;
      }

      // prevent dash spam 2: wait for dash time to elapse
      // this could be replaced by stamina part
      using namespace std::chrono_literals;
      const auto now = std::chrono::high_resolution_clock::now();
      if (now - _last_dash_time_point < 1s)
      {
         return;
      }

      // prevent dash spam 3: check player stamina
      if (!SaveState::getPlayerInfo()._extra_table._health.hasFullStamina())
      {
         return;
      }

      _last_dash_time_point = now;

      // first dash iteration
      _frame_count = PhysicsConfiguration::getInstance()._player_dash_frame_count;
      _multiplier = PhysicsConfiguration::getInstance()._player_dash_multiplier;
      _direction = dir;

      // play dash sound
      Audio::getInstance().playSample({"player_dash_01.wav"});

      keepLinearVelocity();
      input.player_body->SetGravityScale(0.0);

      // drain stamina bar
      SaveState::getPlayerInfo()._extra_table._health.addStaminaDrain(Health::StaminaDrain::Dash);
   }

   if (!hasMoreFrames() || _direction == Dash::None)
   {
      return;
   }

   // apply force to player
   const auto left = (_direction == Dash::Left);
   input._points_to_left = left;

   _multiplier += PhysicsConfiguration::getInstance()._player_dash_multiplier_increment_per_frame;
   _multiplier *= PhysicsConfiguration::getInstance()._player_dash_multiplier_scale_per_frame;

   const auto dash_vector = _multiplier * input.player_body->GetMass() * PhysicsConfiguration::getInstance()._player_dash_vector;
   const auto impulse = (left) ? -dash_vector : dash_vector;

   input.player_body->ApplyForceToCenter(b2Vec2(impulse, 0.0f), false);

   _frame_count--;

   if (!hasMoreFrames())
   {
      reset(input.player_body);
   }
}

void PlayerDash::abort()
{
   _frame_count = 0;
}

bool PlayerDash::hasMoreFrames() const
{
   return (_frame_count > 0);
}

void PlayerDash::reset(b2Body* player_body)
{
   // stop draining stamina bar
   SaveState::getPlayerInfo()._extra_table._health.removeStaminaDrain(Health::StaminaDrain::Dash);
   player_body->SetGravityScale(1.0f);
   _reset_dash_callback();
}
