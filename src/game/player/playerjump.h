#pragma once

#include "game/player/playercontrols.h"

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include <chrono>
#include <functional>

class b2Body;
class b2Joint;

/// \brief controls jumping, buffered jumps, wall slide, wall jump, and double jump behavior.
struct PlayerJump
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief constructs jump state with default counters and no active jump mode.
   PlayerJump() = default;

   /// \brief assigns the controls object used to query jump and movement input.
   /// \param newControls shared player controls instance.
   void setControls(const std::shared_ptr<PlayerControls>& newControls);

   /// \brief frame context passed from player state into jump logic.
   struct PlayerJumpInfo
   {
      bool _in_air = false;
      bool _in_water = false;
      HighResTimePoint _water_entered_timepoint;
      bool _crouching = false;
      bool _climbing = false;
      bool _dashing = false;
   };

   enum class DustAnimationType
   {
      Ground,
      InAir,
   };

   /// \brief handles a jump button press, including regular jump, jump buffering, wall jump, and double jump.
   void jump();
   /// \brief applies a fixed upward impulse-based jump.
   void jumpImpulse();
   /// \brief applies a caller-provided jump impulse vector.
   /// \param impulse impulse vector applied to the player body.
   void jumpImpulse(const b2Vec2& impulse);
   /// \brief starts a force-driven jump by initializing jump frame counters.
   void jumpForce();
   /// \brief performs a mid-air double jump when the skill is unlocked and not yet consumed.
   void doubleJump();
   /// \brief starts a wall jump from wall-slide state and locks directional input briefly.
   void wallJump();

   /// \brief advances jump-related state transitions for this frame.
   /// \param info current player movement and environment state.
   void update(const PlayerJumpInfo& info);

   /// \brief executes buffered jump input shortly after landing.
   void updateJumpBuffer();
   /// \brief applies upward jump forces while jump conditions remain valid.
   void updateJump();
   /// \brief maintains coyote-time state after losing ground contact.
   void updateLostGroundContact();
   /// \brief advances wall-slide detection, friction force, and looping wall-slide audio.
   void updateWallSlide();
   /// \brief applies per-frame force for an active wall jump.
   void updateWallJump();

   /// \brief reports whether jump-force frames are still active.
   /// \return true when the regular jump force sequence is still running.
   bool isJumping() const;
   /// \brief reports whether the player is currently in wall-slide state.
   /// \return true when wall sliding is active and no wall jump is currently playing.
   bool isWallSliding() const;

   PlayerJumpInfo _jump_info;
   std::shared_ptr<PlayerControls> _controls;
   b2Body* _body = nullptr;

   sf::Clock _jump_clock;               // replace by chrono
   sf::Time _last_jump_press_time;      // replace by chrono
   sf::Time _ground_contact_lost_time;  // replace by chrono

   HighResTimePoint _timepoint_wallslide;
   HighResTimePoint _timepoint_walljump;
   HighResTimePoint _timepoint_doublejump;

   int32_t _jump_frame_count = 0;
   int32_t _walljump_frame_count = 0;
   float _walljump_multiplier = 0.0f;
   b2Vec2 _walljump_direction;
   bool _walljump_points_right = false;
   std::optional<int32_t> _wallslide_sample;

   bool _had_ground_contact = true;
   bool _ground_contact_just_lost = false;
   bool _wallsliding = false;
   bool _compensate_velocity = false;
   bool _double_jump_consumed = false;

   std::function<void(DustAnimationType)> _jump_dust_animation_callback;
   std::function<void(void)> _remove_climb_joint_callback;
};
