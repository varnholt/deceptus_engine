#pragma once

#include "playercontrols.h"

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include <functional>

class b2Body;
class b2Joint;

struct PlayerJump
{
   PlayerJump() = default;

   struct PlayerJumpInfo
   {
      bool _in_air = false;
      bool _in_water = false;
      bool _crouching = false;
      bool _climbing = false;
   };

   void jump();
   void jumpImpulse();
   void jumpImpulse(const b2Vec2& impulse);
   void jumpForce();
   void doubleJump();
   void wallJump();

   void update(const PlayerJumpInfo& info, const PlayerControls& controls);

   void updateJumpBuffer();
   void updateJump();
   void updateLostGroundContact();
   void updateWallSlide();
   void updateWallJump();

   bool isJumping() const;

   PlayerJumpInfo _jump_info;
   PlayerControls _controls;
   b2Body* _body = nullptr;

   sf::Clock _jump_clock;
   sf::Time _last_jump_press_time;
   sf::Time _ground_contact_lost_time;

   int32_t _jump_steps = 0;
   int32_t _walljump_steps = 0;
   b2Vec2 _walljump_direction;

   bool _had_ground_contact = true;
   bool _ground_contact_just_lost = false;
   bool _wallsliding = false;
   bool _compensate_velocity = false;
   bool _double_jump_consumed = false;

   std::function<void(void)> _dust_animation_callback;
   std::function<void(void)> _remove_climb_joint_callback;
};

