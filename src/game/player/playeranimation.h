#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <unordered_map>

#include "animation.h"
#include "constants.h"

class PlayerAnimation
{
public:
   PlayerAnimation();
   void loadAnimations();

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
      bool _attacking = false;
      DeathReason _death_reason = DeathReason::Invalid;
      WeaponType _weapon_type = WeaponType::None;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};
      int32_t _jump_frame_count = 0;
      int32_t _dash_frame_count = 0;
      HighResTimePoint _timepoint_wallslide;
      HighResTimePoint _timepoint_walljump;
      HighResTimePoint _timepoint_doublejump;
      HighResTimePoint _timepoint_bend_down_start;
      HighResTimePoint _timepoint_bend_down_end;
      HighResTimePoint _timepoint_attack_start;
   };

   void update(const sf::Time& dt, const PlayerAnimationData& data);

   int32_t getJumpAnimationReference() const;

   std::shared_ptr<Animation> getCurrentCycle() const;
   HighResDuration getRevealDuration() const;
   HighResDuration getSwordAttackDurationStanding() const;
   HighResDuration getSwordAttackDurationBendingDown() const;
   std::optional<HighResDuration> getActiveAttackCycleDuration();

   void resetAlpha();

private:

   const std::shared_ptr<Animation>& getMappedArmedAnimation(const std::shared_ptr<Animation>& animation, const PlayerAnimationData& animation_data);

   std::shared_ptr<Animation> _idle_r;
   std::shared_ptr<Animation> _idle_l;

   std::shared_ptr<Animation> _idle_blink_r;
   std::shared_ptr<Animation> _idle_blink_l;
   std::shared_ptr<Animation> _sword_idle_blink_r;
   std::shared_ptr<Animation> _sword_idle_blink_l;

   std::shared_ptr<Animation> _sword_idle_l;
   std::shared_ptr<Animation> _sword_idle_r;
   std::shared_ptr<Animation> _idle_r_tmp;
   std::shared_ptr<Animation> _idle_l_tmp;

   std::shared_ptr<Animation> _bend_down_idle_l;
   std::shared_ptr<Animation> _bend_down_idle_r;
   std::shared_ptr<Animation> _bend_down_idle_blink_l;
   std::shared_ptr<Animation> _bend_down_idle_blink_r;
   std::shared_ptr<Animation> _sword_bend_down_idle_l;
   std::shared_ptr<Animation> _sword_bend_down_idle_r;
   std::shared_ptr<Animation> _sword_bend_down_idle_blink_l;
   std::shared_ptr<Animation> _sword_bend_down_idle_blink_r;
   std::shared_ptr<Animation> _bend_down_idle_l_tmp;
   std::shared_ptr<Animation> _bend_down_idle_r_tmp;

   std::shared_ptr<Animation> _bend_down_r;
   std::shared_ptr<Animation> _bend_down_l;
   std::shared_ptr<Animation> _sword_bend_down_l;
   std::shared_ptr<Animation> _sword_bend_down_r;
   std::shared_ptr<Animation> _bend_up_r;
   std::shared_ptr<Animation> _bend_up_l;
   std::shared_ptr<Animation> _sword_bend_up_l;
   std::shared_ptr<Animation> _sword_bend_up_r;

   std::shared_ptr<Animation> _idle_to_run_r;
   std::shared_ptr<Animation> _idle_to_run_l;
   std::shared_ptr<Animation> _runstop_r;
   std::shared_ptr<Animation> _runstop_l;
   std::shared_ptr<Animation> _run_r;
   std::shared_ptr<Animation> _run_l;
   std::shared_ptr<Animation> _sword_run_l;
   std::shared_ptr<Animation> _sword_run_r;

   std::shared_ptr<Animation> _dash_init_r;
   std::shared_ptr<Animation> _dash_init_l;
   std::shared_ptr<Animation> _dash_r;
   std::shared_ptr<Animation> _dash_l;
   std::shared_ptr<Animation> _dash_stop_r;
   std::shared_ptr<Animation> _dash_stop_l;
   std::shared_ptr<Animation> _sword_dash_init_r;
   std::shared_ptr<Animation> _sword_dash_init_l;
   std::shared_ptr<Animation> _sword_dash_r;
   std::shared_ptr<Animation> _sword_dash_l;
   std::shared_ptr<Animation> _sword_dash_stop_r;
   std::shared_ptr<Animation> _sword_dash_stop_l;

   std::shared_ptr<Animation> _crouch_r;
   std::shared_ptr<Animation> _crouch_l;

   std::shared_ptr<Animation> _jump_init_r;
   std::shared_ptr<Animation> _jump_up_r;
   std::shared_ptr<Animation> _jump_midair_r;
   std::shared_ptr<Animation> _jump_down_r;
   std::shared_ptr<Animation> _jump_landing_r;

   std::shared_ptr<Animation> _jump_init_l;
   std::shared_ptr<Animation> _jump_up_l;
   std::shared_ptr<Animation> _jump_midair_l;
   std::shared_ptr<Animation> _jump_down_l;
   std::shared_ptr<Animation> _jump_landing_l;

   std::shared_ptr<Animation> _sword_jump_init_r;
   std::shared_ptr<Animation> _sword_jump_up_r;
   std::shared_ptr<Animation> _sword_jump_midair_r;
   std::shared_ptr<Animation> _sword_jump_down_r;
   std::shared_ptr<Animation> _sword_jump_landing_r;

   std::shared_ptr<Animation> _sword_jump_init_l;
   std::shared_ptr<Animation> _sword_jump_up_l;
   std::shared_ptr<Animation> _sword_jump_midair_l;
   std::shared_ptr<Animation> _sword_jump_down_l;
   std::shared_ptr<Animation> _sword_jump_landing_l;

   std::shared_ptr<Animation> _double_jump_r;
   std::shared_ptr<Animation> _double_jump_l;

   std::shared_ptr<Animation> _swim_idle_r;
   std::shared_ptr<Animation> _swim_idle_l;
   std::shared_ptr<Animation> _swim_r;
   std::shared_ptr<Animation> _swim_l;
   std::shared_ptr<Animation> _sword_swim_idle_r;
   std::shared_ptr<Animation> _sword_swim_idle_l;
   std::shared_ptr<Animation> _sword_swim_r;
   std::shared_ptr<Animation> _sword_swim_l;

   std::shared_ptr<Animation> _wallslide_impact_r;
   std::shared_ptr<Animation> _wallslide_impact_l;
   std::shared_ptr<Animation> _wallslide_r;
   std::shared_ptr<Animation> _wallslide_l;

   std::shared_ptr<Animation> _wall_jump_r;
   std::shared_ptr<Animation> _wall_jump_l;

   std::shared_ptr<Animation> _appear_r;
   std::shared_ptr<Animation> _appear_l;
   std::shared_ptr<Animation> _sword_appear_r;
   std::shared_ptr<Animation> _sword_appear_l;

   std::shared_ptr<Animation> _sword_bend_down_attack_1_l;
   std::shared_ptr<Animation> _sword_bend_down_attack_1_r;
   std::shared_ptr<Animation> _sword_bend_down_attack_2_l;
   std::shared_ptr<Animation> _sword_bend_down_attack_2_r;
   std::array<std::shared_ptr<Animation>, 1> _sword_standing_attack_l;
   std::array<std::shared_ptr<Animation>, 1> _sword_standing_attack_r;
   std::shared_ptr<Animation> _sword_standing_attack_tmp_l;
   std::shared_ptr<Animation> _sword_standing_attack_tmp_r;
   bool _sword_standing_attack_l_reset = false;
   bool _sword_standing_attack_r_reset = false;

   std::shared_ptr<Animation> _death_default;
   std::shared_ptr<Animation> _death_electrocuted_l;
   std::shared_ptr<Animation> _death_electrocuted_r;

   int32_t _jump_animation_reference = 0;

   std::shared_ptr<Animation> _current_cycle;

   std::vector<std::shared_ptr<Animation>> _looped_animations;
   std::unordered_map<std::shared_ptr<Animation>, std::shared_ptr<Animation>> _sword_lut;
   std::vector<std::shared_ptr<Animation>> _appear_animations;
};
