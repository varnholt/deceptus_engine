#include "playerjump.h"

#include "animationpool.h"
#include "audio.h"
#include "camerapane.h"
#include "framework/tools/globalclock.h"
#include "gamecontactlistener.h"
#include "physics/physicsconfiguration.h"
#include "savestate.h"

#include <Box2D/Box2D.h>
#include <iostream>


namespace
{
constexpr auto minimum_jump_interval_ms = 150;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::update(const PlayerJumpInfo& info, const PlayerControls& controls)
{
   _jump_info = info;
   _controls = controls;

#ifdef JUMP_GRAVITY_SCALING
   if (_jump_info._in_air && !_jump_info._in_air)
   {
      // std::cout << "reset" << std::endl;
      _body->SetGravityScale(1.0f);
   }

   if (_jump_info._in_water)
   {
      _body->SetGravityScale(0.5f);
   }
#endif

   if (!_jump_info._in_air)
   {
      _double_jump_consumed = false;
   }

   updateLostGroundContact();
   updateJump();
   updateJumpBuffer();
   updateWallSlide();
   updateWallJump();
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateJumpBuffer()
{
   if (_jump_info._in_air)
   {
      return;
   }

   // if jump is pressed while the ground is just a few centimeters away,
   // store the information and jump as soon as the places touches ground
   auto now = GlobalClock::getInstance()->getElapsedTime();
   auto time_diff = (now - _last_jump_press_time).asMilliseconds();

   if (time_diff < PhysicsConfiguration::getInstance().mPlayerJumpBufferMs)
   {
      jump();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateJump()
{
   if (_jump_info._in_water && _controls.isJumpButtonPressed())
   {
      _body->ApplyForce(b2Vec2(0, -1.0f), _body->GetWorldCenter(), true);
   }
   else if (
         (_jump_frame_count > 0 && _controls.isJumpButtonPressed())
      || _jump_clock.getElapsedTime().asMilliseconds() < PhysicsConfiguration::getInstance().mPlayerJumpMinimalDurationMs
   )
   {
      // probably dead code

      // jump higher if faster than regular walk speed
      auto max_walk = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
      auto vel = fabs(_body->GetLinearVelocity().x) - max_walk;
      auto factor = 1.0f;

      if (vel > 0.0f)
      {
         auto max_run = PhysicsConfiguration::getInstance().mPlayerSpeedMaxRun;

         factor =
              1.0f
            + PhysicsConfiguration::getInstance().mPlayerJumpSpeedFactor * (vel / (max_run - max_walk));
      }

      /*
       * +---+
         |###|
         |###| <- current speed => factor
         |###|
         +###+
         |   |
         |   |
         |   |
       * +---+
      */

      // to change velocity by 5 in one time step
      constexpr auto fixed_timestep = (1.0f / 60.0f);

      // f = mv / t
      auto force = factor * _body->GetMass() * PhysicsConfiguration::getInstance().mPlayerJumpStrength / fixed_timestep;

      // spread the force over 6.5 time steps
      force /= PhysicsConfiguration::getInstance().mPlayerJumpFalloff;

      // more force is required to compensate falling velocity for scenarios
      // - wall jump
      // - double jump
      if (_compensate_velocity)
      {
         const auto bodyVelocity = _body->GetLinearVelocity();
         force *= 1.75f * bodyVelocity.y;

         _compensate_velocity = false;
      }

      // printf("force: %f\n", force);
      _body->ApplyForceToCenter(b2Vec2(0.0f, -force), true);

      _jump_frame_count--;

      if (_jump_frame_count == 0)
      {
         // could be a bug
         // all other code sets the gravity scale back to 1.0

         _body->SetGravityScale(1.35f);
      }
   }
   else
   {
      _jump_frame_count = 0;
   }
}


//----------------------------------------------------------------------------------------------------------------------
// not used by the game
void PlayerJump::jumpImpulse()
{
   _jump_clock.restart();

   float impulse = _body->GetMass() * 6.0f;

   _body->ApplyLinearImpulse(
      b2Vec2(0.0f, -impulse),
      _body->GetWorldCenter(),
      true
   );
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::jumpImpulse(const b2Vec2& impulse)
{
   _jump_clock.restart();

   _body->ApplyLinearImpulse(
      impulse,
      _body->GetWorldCenter(),
      true
   );
}


//----------------------------------------------------------------------------------------------------------------------
// apply individual forces for a given number of frames
// that's the approach this game is currently using
void PlayerJump::jumpForce()
{
   _jump_clock.restart();
   _jump_frame_count = PhysicsConfiguration::getInstance().mPlayerJumpSteps;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::doubleJump()
{
   if (_walljump_frame_count > 0)
   {
      return;
   }

   const auto skills = SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills;
   const auto canDoubleJump = (skills & ExtraSkill::SkillDoubleJump);

   if (!canDoubleJump)
   {
      return;
   }

   if (_double_jump_consumed)
   {
      return;
   }

   _double_jump_consumed = true;
   _compensate_velocity = true;

   // double jump should happen with a constant impulse, no adjusting through button press duration
   const auto current_velocity = _body->GetLinearVelocity();
   _body->SetLinearVelocity(b2Vec2(current_velocity.x, 0.0f));
   jumpImpulse(b2Vec2(0.0f, _body->GetMass() * PhysicsConfiguration::getInstance().mPlayerDoubleJumpFactor));

   // old approach, can probably be removed
   // jumpForce();
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::wallJump()
{
   const auto skills = SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills;
   const auto canWallJump = (skills & ExtraSkill::SkillWallJump);

   if (!canWallJump)
   {
      return;
   }

   if (!_wallsliding)
   {
      return;
   }

   const auto jump_right = (GameContactListener::getInstance()->getNumArmLeftContacts() > 0);

   // double jump should happen with a constant impulse, no adjusting through button press duration
   _body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));

   const auto impulse_x =   _body->GetMass() * PhysicsConfiguration::getInstance().mPlayerWallJumpVectorX;
   const auto impulse_y = -(_body->GetMass() * PhysicsConfiguration::getInstance().mPlayerWallJumpVectorY);

   _walljump_frame_count = PhysicsConfiguration::getInstance().mPlayerWallJumpFrameCount;
   _walljump_multiplier = PhysicsConfiguration::getInstance().mPlayerWallJumpMultiplier;
   _walljump_direction = b2Vec2(jump_right ? impulse_x : -impulse_x, impulse_y);

   // old approach, can probably be removed
   // jumpForce();
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::jump()
{
   if (_jump_info._crouching)
   {
      return;
   }

   if (CameraPane::getInstance().isLookActive())
   {
      return;
   }

   sf::Time elapsed = _jump_clock.getElapsedTime();

   // only allow a new jump after a a couple of milliseconds
   if (elapsed.asMilliseconds() > minimum_jump_interval_ms)
   {
      // handle regular jump
      if (!_jump_info._in_air || _ground_contact_just_lost || _jump_info._climbing)
      {
         _remove_climb_joint_callback();
         _jump_info._climbing = false; // only set for correctness until next frame

         jumpForce();

         if (_jump_info._in_water)
         {
            // play some waterish sample?
         }
         else
         {
            _dust_animation_callback();
            Audio::getInstance()->playSample("jump.wav");
         }
      }
      else
      {
         // player pressed jump but is still in air.
         // buffer that information to trigger the jump a few millis later.
         if (_jump_info._in_air)
         {
            _last_jump_press_time = GlobalClock::getInstance()->getElapsedTime();

            // handle wall jump
            wallJump();

            // handle double jump
            doubleJump();
         }
      }
   }
}



//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateLostGroundContact()
{
   // when losing contact to the ground allow jumping for 2-3 more frames
   //
   // if player had ground contact in previous frame but now lost ground
   // contact then start counting to 200ms
   if (_had_ground_contact && _jump_info._in_air && !isJumping())
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      _ground_contact_lost_time = now;
      _ground_contact_just_lost = true;
   }

   // flying now, probably allow jump
   else if (_jump_info._in_air)
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      auto timeDiff = (now - _ground_contact_lost_time).asMilliseconds();
      _ground_contact_just_lost = (timeDiff < PhysicsConfiguration::getInstance().mPlayerJumpAfterContactLostMs);

      // if (mGroundContactJustLost)
      // {
      //    std::cout << "allowed to jump for another " << timeDiff << "ms" << std::endl;
      // }
   }
   else
   {
      _ground_contact_just_lost = false;
   }

   _had_ground_contact = !_jump_info._in_air;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateWallSlide()
{
   if (!_jump_info._in_air)
   {
      _wallsliding = false;
      return;
   }

   // early out if walljump still active
   if (_walljump_frame_count > 0)
   {
      return;
   }

   const auto skills = SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills;
   const auto canWallSlide = (skills & ExtraSkill::SkillWallSlide);

   if (!canWallSlide)
   {
      _wallsliding = false;
      return;
   }

   const auto leftTouching = (GameContactListener::getInstance()->getNumArmLeftContacts() > 0);
   const auto rightTouching = (GameContactListener::getInstance()->getNumArmRightContacts() > 0);

   if (
         !(leftTouching  && _controls.isMovingLeft())
      && !(rightTouching && _controls.isMovingRight())
   )
   {
      _wallsliding = false;
      return;
   }

   b2Vec2 vel = _body->GetLinearVelocity();
   _body->ApplyForce(PhysicsConfiguration::getInstance().mPlayerWallSlideFriction * -vel, _body->GetWorldCenter(), false);
   _wallsliding = true;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateWallJump()
{
   if (_walljump_frame_count == 0)
   {
      return;
   }

   _walljump_multiplier *= PhysicsConfiguration::getInstance().mPlayerWallJumpMultiplierScalePerFrame;
   _walljump_multiplier += PhysicsConfiguration::getInstance().mPlayerWallJumpMultiplierIncrementPerFrame;

   _body->ApplyForceToCenter(_walljump_multiplier * _walljump_direction, true);

   // std::cout << "step: " << _walljump_steps << " " << _walljump_direction.x << " " << _walljump_direction.y << std::endl;

   _walljump_frame_count--;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerJump::isJumping() const
{
   return (_jump_frame_count > 0);
}
