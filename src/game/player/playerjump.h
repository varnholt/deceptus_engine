#pragma once

#include "game/player/playercontrols.h"

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>

#include <chrono>
#include <functional>

class b2Body;
class b2Joint;

struct PlayerJump
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   PlayerJump() = default;

   void setControls(const std::shared_ptr<PlayerControls>& newControls);

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

   void jump();
   void jumpImpulse();
   void jumpImpulse(const b2Vec2& impulse);
   void jumpForce();
   void doubleJump();
   void wallJump();

   void update(const PlayerJumpInfo& info);

   void updateJumpBuffer();
   void updateJump();
   void updateLostGroundContact();
   void updateWallSlide();
   void updateWallJump();

   bool isJumping() const;

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
