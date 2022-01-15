#pragma once

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <optional>

#include "animation.h"
#include "constants.h"
#include "playercontrols.h"
#include "playerjump.h"

class PlayerAnimation
{

public:

   PlayerAnimation();

   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   struct PlayerAnimationData
   {
      bool _dead = false;
      bool _in_air = false;
      bool _in_water = false;
      bool _hard_landing = false;
      bool _bending_down = false;
      bool _crouching = false;
      bool _points_left = false;
      bool _points_right = false;
      bool _climb_joint_present = false;
      bool _moving_left = false;
      bool _moving_right = false;
      bool _wall_sliding = false;
      bool _wall_jump_points_right = false;
      bool _jumping_through_one_way_wall = false;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};
      int32_t _jump_frame_count = 0;
      int32_t _dash_frame_count = 0;
      HighResTimePoint _timepoint_wallslide;
      HighResTimePoint _timepoint_walljump;
      HighResTimePoint _timepoint_doublejump;
      HighResTimePoint _timepoint_bend_down_start;
      HighResTimePoint _timepoint_bend_down_end;
   };

   void update(
      const sf::Time& dt,
      const PlayerAnimationData& data
   );

   int32_t getJumpAnimationReference() const;

   std::shared_ptr<Animation> getCurrentCycle() const;
   HighResDuration getRevealDuration() const;

   void resetAlpha();

   static void generateJson();


private:

   std::shared_ptr<Animation> _idle_r_2;
   std::shared_ptr<Animation> _idle_l_2;
   std::shared_ptr<Animation> _idle_blink_r_2;
   std::shared_ptr<Animation> _idle_blink_l_2;
   std::shared_ptr<Animation> _idle_r_tmp;
   std::shared_ptr<Animation> _idle_l_tmp;

   std::shared_ptr<Animation> _bend_down_idle_l_2;
   std::shared_ptr<Animation> _bend_down_idle_r_2;
   std::shared_ptr<Animation> _bend_down_idle_blink_l_2;
   std::shared_ptr<Animation> _bend_down_idle_blink_r_2;
   std::shared_ptr<Animation> _bend_down_idle_l_tmp;
   std::shared_ptr<Animation> _bend_down_idle_r_tmp;

   std::shared_ptr<Animation> _bend_down_r_2;
   std::shared_ptr<Animation> _bend_down_l_2;
   std::shared_ptr<Animation> _bend_up_r_2;
   std::shared_ptr<Animation> _bend_up_l_2;

   std::shared_ptr<Animation> _idle_to_run_r_2;
   std::shared_ptr<Animation> _idle_to_run_l_2;
   std::shared_ptr<Animation> _runstop_r_2;
   std::shared_ptr<Animation> _runstop_l_2;
   std::shared_ptr<Animation> _run_r_2;
   std::shared_ptr<Animation> _run_l_2;

   std::shared_ptr<Animation> _dash_init_r_2;
   std::shared_ptr<Animation> _dash_init_l_2;
   std::shared_ptr<Animation> _dash_r_2;
   std::shared_ptr<Animation> _dash_l_2;
   std::shared_ptr<Animation> _dash_stop_r_2;
   std::shared_ptr<Animation> _dash_stop_l_2;

   std::shared_ptr<Animation> _crouch_r_2;
   std::shared_ptr<Animation> _crouch_l_2;

   std::shared_ptr<Animation> _jump_init_r_2;
   std::shared_ptr<Animation> _jump_up_r_2;
   std::shared_ptr<Animation> _jump_midair_r_2;
   std::shared_ptr<Animation> _jump_down_r_2;
   std::shared_ptr<Animation> _jump_landing_r_2;

   std::shared_ptr<Animation> _jump_init_l_2;
   std::shared_ptr<Animation> _jump_up_l_2;
   std::shared_ptr<Animation> _jump_midair_l_2;
   std::shared_ptr<Animation> _jump_down_l_2;
   std::shared_ptr<Animation> _jump_landing_l_2;

   std::shared_ptr<Animation> _double_jump_r_2;
   std::shared_ptr<Animation> _double_jump_l_2;
   std::shared_ptr<Animation> _swim_idle_r_2;
   std::shared_ptr<Animation> _swim_idle_l_2;
   std::shared_ptr<Animation> _swim_r_2;
   std::shared_ptr<Animation> _swim_l_2;

   std::shared_ptr<Animation> _wallslide_impact_r_2;
   std::shared_ptr<Animation> _wallslide_impact_l_2;
   std::shared_ptr<Animation> _wallslide_r_2;
   std::shared_ptr<Animation> _wallslide_l_2;
   std::shared_ptr<Animation> _wall_jump_r_2;
   std::shared_ptr<Animation> _wall_jump_l_2;
   std::shared_ptr<Animation> _appear_r_2;
   std::shared_ptr<Animation> _appear_l_2;

   std::shared_ptr<Animation> _death;

   int32_t _jump_animation_reference = 0;

   std::vector<std::shared_ptr<Animation>> _looped_animations;
   std::shared_ptr<Animation> _current_cycle;
};

