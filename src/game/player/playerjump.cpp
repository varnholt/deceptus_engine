#include "game/player/playerjump.h"

#include "framework/tools/globalclock.h"
#include "framework/tools/scopeexit.h"
#include "framework/tools/stopwatch.h"
#include "game/audio/audio.h"
#include "game/physics/gamecontactlistener.h"
#include "game/physics/physicsconfiguration.h"
#include "game/state/displaymode.h"
#include "game/state/savestate.h"

#include <box2d/box2d.h>

namespace
{
constexpr auto fixed_timestep = (1.0f / 60.0f);
}

void PlayerJump::update(const PlayerJumpInfo& info)
{
   const auto was_in_air = _jump_info._in_air;

   _jump_info = info;

   if (was_in_air && !_jump_info._in_air)
   {
      _body->SetGravityScale(PhysicsConfiguration::getInstance()._gravity_scale_default);

      // we don't want to play the landing sample when just trespassing one-way-walls
      // value below found just by measurent the player speed and setting something suitable
      // std::cout << _body->GetLinearVelocity().y << std::endl;
      if (_body->GetLinearVelocity().y > -2.0f)
      {
         Audio::getInstance().playSample({"player_jump_land.wav", 0.5f});
      }
   }

   if (_jump_info._in_water)
   {
      _body->SetGravityScale(PhysicsConfiguration::getInstance()._gravity_scale_water);
   }

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

void PlayerJump::updateJumpBuffer()
{
   if (_jump_info._in_air)
   {
      return;
   }

   // if jump is pressed while the ground is just a few centimeters away,
   // store the information and jump as soon as the places touches ground
   auto now = GlobalClock::getInstance().getElapsedTime();
   auto time_diff = (now - _last_jump_press_time).asMilliseconds();

   if (time_diff < PhysicsConfiguration::getInstance()._player_jump_buffer_ms)
   {
      jump();
   }
}

void PlayerJump::updateJump()
{
   const auto& physics = PhysicsConfiguration::getInstance();

   if (_jump_info._in_water && _controls->isButtonAPressed())
   {
      // only allow jumping out of the water / water movement if the player stayed inside the water for a bit
      using namespace std::chrono_literals;
      const auto time_in_water = StopWatch::getInstance().now() - _jump_info._water_entered_timepoint;
      const auto time_to_allow_up = std::chrono::milliseconds(physics._player_in_water_time_to_allow_jump_button_ms);
      if (time_in_water > time_to_allow_up)
      {
         _body->ApplyForce(b2Vec2{0, physics._player_in_water_force_jump_button}, _body->GetWorldCenter(), true);

         // to transition to a regular jump after leaving the water, the jump frame count and jump clock should be reset
         _jump_frame_count = physics._player_jump_frame_count;
         _jump_clock.restart();
      }
   }
   else if ((_jump_frame_count > 0 && _controls->isButtonAPressed())  // still jumping (button pressed)
            || (_jump_frame_count > 0 && (physics._player_jump_frame_count - _jump_frame_count < physics._player_jump_frame_count_minimum)
               )  // still jumping (minimum jump frames)
            || _jump_clock.getElapsedTime().asMilliseconds() < physics._player_jump_minimal_duration_ms  // fresh jump
   )
   {
      // jump higher if faster than regular walk speed
      const auto max_walk = physics._player_speed_max_walk;
      const auto linear_velocity = fabs(_body->GetLinearVelocity().x) - max_walk;
      auto factor = 1.0f;

      if (linear_velocity > 0.0f)
      {
         // probably dead code
         const auto max_run = physics._player_speed_max_run;
         factor = 1.0f + physics._player_jump_speed_factor * (linear_velocity / (max_run - max_walk));
      }

      // f = mv / t
      // spread the force over the configured number of time steps
      auto force = factor * _body->GetMass() * physics._player_jump_strength / fixed_timestep;
      force /= physics._player_jump_falloff;

      // more force is required to compensate falling velocity for scenarios 'wall jump', 'double jump'
      if (_compensate_velocity)
      {
         const auto body_velocity = _body->GetLinearVelocity();
         force *= physics._player_wall_jump_extra_force * body_velocity.y;
         _compensate_velocity = false;
      }

      _body->ApplyForceToCenter(b2Vec2(0.0f, -force), true);

      _jump_frame_count--;
      if (_jump_frame_count == 0)
      {
         // out of 'push upward'-frames, now it goes down quickly until player hits ground
         _body->SetGravityScale(physics._gravity_scale_jump_downward);
      }
   }
   else
   {
      _jump_frame_count = 0;
   }
}

void PlayerJump::jumpImpulse()
{
   const auto impulse = _body->GetMass() * PhysicsConfiguration::getInstance()._player_jump_impulse_factor;

   _jump_clock.restart();
   _body->ApplyLinearImpulse(b2Vec2(0.0f, -impulse), _body->GetWorldCenter(), true);
}

void PlayerJump::jumpImpulse(const b2Vec2& impulse)
{
   _jump_clock.restart();
   _body->ApplyLinearImpulse(impulse, _body->GetWorldCenter(), true);
}

void PlayerJump::jumpForce()
{
   // apply individual forces for a given number of frames
   // that's the approach this game is currently using
   _jump_clock.restart();
   _jump_frame_count = PhysicsConfiguration::getInstance()._player_jump_frame_count;
}

void PlayerJump::doubleJump()
{
   if (_walljump_frame_count > 0)
   {
      return;
   }

   const auto skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
   const auto can_double_jump = (skills & static_cast<int32_t>(Skill::SkillType::DoubleJump));

   if (!can_double_jump)
   {
      return;
   }

   if (_double_jump_consumed)
   {
      return;
   }

   _double_jump_consumed = true;
   _compensate_velocity = true;

   // double jump also has dust
   _jump_dust_animation_callback(DustAnimationType::InAir);

   // double jump should happen with a constant impulse, no adjusting through button press duration
   const auto current_velocity = _body->GetLinearVelocity();
   _body->SetLinearVelocity(b2Vec2(current_velocity.x, 0.0f));
   jumpImpulse(b2Vec2(0.0f, _body->GetMass() * PhysicsConfiguration::getInstance()._player_double_jump_factor));

   _timepoint_doublejump = StopWatch::now();
   Audio::getInstance().playSample({"player_doublejump_01.mp3"});
}

void PlayerJump::wallJump()
{
   const auto skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
   const auto can_wall_jump = (skills & static_cast<int32_t>(Skill::SkillType::WallJump));

   if (!can_wall_jump)
   {
      return;
   }

   if (!_wallsliding)
   {
      return;
   }

   _walljump_points_right = (GameContactListener::getInstance().getPlayerArmLeftContactCount() > 0);

   // double jump should happen with a constant impulse, no adjusting through button press duration
   _body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));

   const auto& physics = PhysicsConfiguration::getInstance();

   const auto impulse_x = _body->GetMass() * physics._player_wall_jump_vector_x;
   const auto impulse_y = -(_body->GetMass() * physics._player_wall_jump_vector_y);

   _walljump_frame_count = physics._player_wall_jump_frame_count;
   _walljump_multiplier = physics._player_wall_jump_multiplier;
   _walljump_direction = b2Vec2(_walljump_points_right ? impulse_x : -impulse_x, impulse_y);
   _timepoint_walljump = StopWatch::now();

   using namespace std::chrono_literals;
   _controls->lockState(
      _walljump_points_right ? KeyPressedRight : KeyPressedLeft,
      PlayerControls::LockedState::Pressed,
      std::chrono::milliseconds(physics._player_wall_jump_lock_key_duration_ms)
   );
   _controls->lockState(
      _walljump_points_right ? KeyPressedLeft : KeyPressedRight,
      PlayerControls::LockedState::Released,
      std::chrono::milliseconds(physics._player_wall_jump_lock_key_duration_ms)
   );
}

void PlayerJump::jump()
{
   if (_jump_info._dashing)
   {
      // don't allow jump when dash is active.
      // with gravity scale set to 0, the player would do super jumps.
      return;
   }

   // using a higher threshold here to avoid blocking jumps for 'no reason'.
   // a 0.3 value for the y axis (default threshold) might not be on purpose
   if (_controls->isMovingDown(0.6f))
   {
      // equivalent to check for bending down, but jump can be called via lamda
      // which mich skip the bend state update in the player update loop.
      return;
   }

   if (DisplayMode::getInstance().isSet(Display::CameraPanorama))  // redundant, can be removed
   {
      return;
   }

   const auto elapsed = _jump_clock.getElapsedTime();

   // only allow a new jump after a a couple of milliseconds
   if (elapsed.asMilliseconds() <= PhysicsConfiguration::getInstance()._player_minimum_jump_interval_ms)
   {
      return;
   }

   // handle regular jump
   if (!_jump_info._in_air || _ground_contact_just_lost || _jump_info._climbing)
   {
      _remove_climb_joint_callback();
      _jump_info._climbing = false;  // only set for correctness until next frame

      jumpForce();

      if (_jump_info._in_water)
      {
         // play some waterish sample?
      }
      else
      {
         _jump_dust_animation_callback(_ground_contact_just_lost ? DustAnimationType::InAir : DustAnimationType::Ground);
         Audio::getInstance().playSample({"player_jump_liftoff.wav", 0.3f});
      }
   }
   else
   {
      // player pressed jump but is still in air.
      // buffer that information to trigger the jump a few millis later.
      if (_jump_info._in_air)
      {
         _last_jump_press_time = GlobalClock::getInstance().getElapsedTime();

         wallJump();    // handle wall jump
         doubleJump();  // handle double jump
      }
   }
}

void PlayerJump::updateLostGroundContact()
{
   // when losing contact to the ground allow jumping for 2-3 more frames
   //
   // if player had ground contact in previous frame but now lost ground
   // contact then start counting to 200ms
   if (_had_ground_contact && _jump_info._in_air && !isJumping())
   {
      const auto now = GlobalClock::getInstance().getElapsedTime();
      _ground_contact_lost_time = now;
      _ground_contact_just_lost = true;
   }

   // flying now, probably allow jump
   else if (_jump_info._in_air)
   {
      const auto now = GlobalClock::getInstance().getElapsedTime();
      const auto time_diff = (now - _ground_contact_lost_time).asMilliseconds();
      _ground_contact_just_lost = (time_diff < PhysicsConfiguration::getInstance()._player_jump_after_contact_lost_ms);
   }
   else
   {
      _ground_contact_just_lost = false;
   }

   _had_ground_contact = !_jump_info._in_air;
}

void PlayerJump::updateWallSlide()
{
   // start and stop wallsliding sample
   ScopeExit scope_exit(
      [this]()
      {
         if (_wallsliding)
         {
            if (!_wallslide_sample.has_value())
            {
               _wallslide_sample = Audio::getInstance().playSample({"player_wallslide_01.wav", 1.0, true});
            }
         }
         else
         {
            if (_wallslide_sample.has_value())
            {
               Audio::getInstance().stopSample(_wallslide_sample.value());
               _wallslide_sample.reset();
            }
         }
      }
   );

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

   const auto skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
   const auto can_wallslide = (skills & static_cast<int32_t>(Skill::SkillType::WallSlide));

   if (!can_wallslide)
   {
      _wallsliding = false;
      return;
   }

   const auto touching_left = (GameContactListener::getInstance().getPlayerArmLeftContactCount() > 0);
   const auto touching_right = (GameContactListener::getInstance().getPlayerArmRightContactCount() > 0);

   if (!(touching_left && _controls->isMovingLeft()) && !(touching_right && _controls->isMovingRight()))
   {
      _wallsliding = false;
      return;
   }

   const auto& linear_velocity = _body->GetLinearVelocity();
   _body->ApplyForce(PhysicsConfiguration::getInstance()._player_wall_slide_friction * -linear_velocity, _body->GetWorldCenter(), false);

   if (!_wallsliding)
   {
      _timepoint_wallslide = StopWatch::now();
      _wallsliding = true;
   }
}

void PlayerJump::updateWallJump()
{
   if (_walljump_frame_count == 0)
   {
      return;
   }

   const auto& physics = PhysicsConfiguration::getInstance();

   _walljump_multiplier *= physics._player_wall_jump_multiplier_scale_per_frame;
   _walljump_multiplier += physics._player_wall_jump_multiplier_increment_per_frame;

   _body->ApplyForceToCenter(_walljump_multiplier * _walljump_direction, true);

   _walljump_frame_count--;
}

bool PlayerJump::isJumping() const
{
   return (_jump_frame_count > 0);
}

void PlayerJump::setControls(const std::shared_ptr<PlayerControls>& controls)
{
   _controls = controls;
}
