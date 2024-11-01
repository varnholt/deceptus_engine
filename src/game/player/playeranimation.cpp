#include "playeranimation.h"

#include <cstdlib>
#include <iostream>
#include <ranges>

#include "framework/tools/log.h"
#include "framework/tools/stopwatch.h"
#include "game/animation/animationpool.h"
#include "game/clock/gameclock.h"
#include "game/mechanisms/portal.h"
#include "game/physics/physicsconfiguration.h"
#include "game/state/displaymode.h"

namespace
{
constexpr auto FRAMES_COUNT_JUMP_INIT = 5;
constexpr auto JUMP_UP_VELOCITY_THRESHOLD = -1.2f;
constexpr auto JUMP_DOWN_VELOCITY_THRESHOLD = 1.2f;

std::chrono::high_resolution_clock::time_point now;

std::ostream& operator<<(std::ostream& os, const PlayerAnimation::PlayerAnimationData& data)
{
   os << "Player Animation Data:\n";
   os << "Dead: " << std::boolalpha << data._dead << std::endl;
   os << "In Air: " << data._in_air << std::endl;
   os << "In Water: " << data._in_water << std::endl;
   os << "Hard Landing: " << data._hard_landing << std::endl;
   os << "Bending Down: " << data._bending_down << std::endl;
   os << "Crouching: " << data._crouching << std::endl;
   os << "Points Left: " << data._points_left << std::endl;
   os << "Points Right: " << data._points_right << std::endl;
   os << "Climb Joint Present: " << data._climb_joint_present << std::endl;
   os << "Moving Left: " << data._moving_left << std::endl;
   os << "Moving Right: " << data._moving_right << std::endl;
   os << "Wall Sliding: " << data._wall_sliding << std::endl;
   os << "Wall Jump Points Right: " << data._wall_jump_points_right << std::endl;
   os << "Jumping Through One-Way Wall: " << data._jumping_through_one_way_wall << std::endl;
   os << "Attacking: " << data._attacking << std::endl;
   os << "Death Reason: " << static_cast<int>(data._death_reason) << std::endl;
   os << "Weapon Type: " << static_cast<int>(data._weapon_type) << std::endl;

   os << "Dash Direction: ";
   if (data._dash_dir.has_value())
   {
      switch (data._dash_dir.value())
      {
         case Dash::Left:
            os << "Left" << std::endl;
            break;
         case Dash::Right:
            os << "Right" << std::endl;
            break;
         case Dash::None:
            os << "None" << std::endl;
            break;
      }
   }
   else
   {
      os << "N/A";
   }
   os << std::endl;

   os << "Linear Velocity: (" << data._linear_velocity.x << ", " << data._linear_velocity.y << ")\n";
   os << "Jump Frame Count: " << data._jump_frame_count << std::endl;
   os << "Dash Frame Count: " << data._dash_frame_count << std::endl;
   os << "Timepoint Wallslide: " << data._timepoint_wallslide.time_since_epoch().count() << std::endl;
   os << "Timepoint Walljump: " << data._timepoint_walljump.time_since_epoch().count() << std::endl;
   os << "Timepoint Doublejump: " << data._timepoint_doublejump.time_since_epoch().count() << std::endl;
   os << "Timepoint Bend Down Start: " << data._timepoint_bend_down_start.time_since_epoch().count() << std::endl;
   os << "Timepoint Bend Down End: " << data._timepoint_bend_down_end.time_since_epoch().count() << std::endl;
   os << "Timepoint Attack Start: " << data._timepoint_attack_start.time_since_epoch().count() << std::endl;
   os << "Timepoint Attack Bend Down Start: " << data._timepoint_attack_bend_down_start.time_since_epoch().count() << std::endl;
   os << "Timepoint Attack Jumping Start: " << data._timepoint_attack_jumping_start.time_since_epoch().count() << std::endl;
   os << "Timepoint Attack Standing Start: " << data._timepoint_attack_standing_start.time_since_epoch().count() << std::endl;

   return os;
}

}  // namespace

void PlayerAnimation::loadAnimations(AnimationPool& pool)
{
   _current_cycle.reset();
   _looped_animations.clear();
   _sword_lut.clear();
   _sword_attack_lut.clear();
   _appear_animations.clear();

   _idle_r = pool.create("player_idle_r", 0.0f, 0.0f, true, false);
   _idle_l = pool.create("player_idle_l", 0.0f, 0.0f, true, false);
   _sword_idle_r = pool.create("player_idle_sword_r", 0.0f, 0.0f, true, false);
   _sword_idle_l = pool.create("player_idle_sword_l", 0.0f, 0.0f, true, false);

   _idle_blink_r = pool.create("player_idle_blink_r", 0.0f, 0.0f, true, false);
   _idle_blink_l = pool.create("player_idle_blink_l", 0.0f, 0.0f, true, false);
   _sword_idle_blink_r = pool.create("player_idle_blink_sword_r", 0.0f, 0.0f, true, false);
   _sword_idle_blink_l = pool.create("player_idle_blink_sword_l", 0.0f, 0.0f, true, false);

   _bend_down_r = pool.create("player_bend_down_r", 0.0f, 0.0f, true, false);
   _bend_down_l = pool.create("player_bend_down_l", 0.0f, 0.0f, true, false);
   _sword_bend_down_r = pool.create("player_bend_down_sword_r", 0.0f, 0.0f, true, false);
   _sword_bend_down_l = pool.create("player_bend_down_sword_l", 0.0f, 0.0f, true, false);

   _bend_up_r = pool.create("player_bend_down_r", 0.0f, 0.0f, true, false);
   _bend_up_l = pool.create("player_bend_down_l", 0.0f, 0.0f, true, false);
   _sword_bend_up_r = pool.create("player_bend_down_sword_r", 0.0f, 0.0f, true, false);
   _sword_bend_up_l = pool.create("player_bend_down_sword_l", 0.0f, 0.0f, true, false);

   _bend_down_idle_r = pool.create("player_bend_down_idle_r", 0.0f, 0.0f, true, false);
   _bend_down_idle_l = pool.create("player_bend_down_idle_l", 0.0f, 0.0f, true, false);
   _sword_bend_down_idle_r = pool.create("player_bend_down_idle_sword_r", 0.0f, 0.0f, true, false);
   _sword_bend_down_idle_l = pool.create("player_bend_down_idle_sword_l", 0.0f, 0.0f, true, false);

   _bend_down_idle_blink_r = pool.create("player_bend_down_idle_blink_r", 0.0f, 0.0f, true, false);
   _bend_down_idle_blink_l = pool.create("player_bend_down_idle_blink_l", 0.0f, 0.0f, true, false);
   _sword_bend_down_idle_blink_r = pool.create("player_bend_down_idle_blink_sword_r", 0.0f, 0.0f, true, false);
   _sword_bend_down_idle_blink_l = pool.create("player_bend_down_idle_blink_sword_l", 0.0f, 0.0f, true, false);

   _idle_to_run_r = pool.create("player_idle_to_run_r", 0.0f, 0.0f, true, false);  // unused
   _idle_to_run_l = pool.create("player_idle_to_run_l", 0.0f, 0.0f, true, false);  // unused
   _runstop_r = pool.create("player_runstop_r", 0.0f, 0.0f, true, false);          // unused
   _runstop_l = pool.create("player_runstop_l", 0.0f, 0.0f, true, false);          // unused

   _run_r = pool.create("player_run_r", 0.0f, 0.0f, true, false);
   _run_l = pool.create("player_run_l", 0.0f, 0.0f, true, false);
   _sword_run_l = pool.create("player_run_sword_l", 0.0f, 0.0f, true, false);
   _sword_run_r = pool.create("player_run_sword_r", 0.0f, 0.0f, true, false);

   _dash_init_r = pool.create("player_dash_init_r", 0.0f, 0.0f, true, false);
   _dash_init_l = pool.create("player_dash_init_l", 0.0f, 0.0f, true, false);
   _dash_r = pool.create("player_dash_r", 0.0f, 0.0f, true, false);
   _dash_l = pool.create("player_dash_l", 0.0f, 0.0f, true, false);
   _dash_stop_r = pool.create("player_dash_init_r", 0.0f, 0.0f, true, false);
   _dash_stop_l = pool.create("player_dash_init_l", 0.0f, 0.0f, true, false);
   _sword_dash_init_r = pool.create("player_dash_init_sword_r", 0.0f, 0.0f, true, false);
   _sword_dash_init_l = pool.create("player_dash_init_sword_l", 0.0f, 0.0f, true, false);
   _sword_dash_r = pool.create("player_dash_sword_r", 0.0f, 0.0f, true, false);
   _sword_dash_l = pool.create("player_dash_sword_l", 0.0f, 0.0f, true, false);
   _sword_dash_stop_r = pool.create("player_dash_init_sword_r", 0.0f, 0.0f, true, false);
   _sword_dash_stop_l = pool.create("player_dash_init_sword_l", 0.0f, 0.0f, true, false);

   _jump_init_r = pool.create("player_jump_init_r", 0.0f, 0.0f, true, false);
   _jump_init_l = pool.create("player_jump_init_l", 0.0f, 0.0f, true, false);
   _jump_up_r = pool.create("player_jump_up_r", 0.0f, 0.0f, true, false);
   _jump_up_l = pool.create("player_jump_up_l", 0.0f, 0.0f, true, false);
   _jump_midair_r = pool.create("player_jump_midair_r", 0.0f, 0.0f, true, false);
   _jump_midair_l = pool.create("player_jump_midair_l", 0.0f, 0.0f, true, false);
   _jump_down_r = pool.create("player_jump_down_r", 0.0f, 0.0f, true, false);
   _jump_down_l = pool.create("player_jump_down_l", 0.0f, 0.0f, true, false);
   _jump_landing_r = pool.create("player_jump_landing_r", 0.0f, 0.0f, true, false);
   _jump_landing_l = pool.create("player_jump_landing_l", 0.0f, 0.0f, true, false);

   _sword_jump_init_r = pool.create("player_jump_init_sword_r", 0.0f, 0.0f, true, false);
   _sword_jump_init_l = pool.create("player_jump_init_sword_l", 0.0f, 0.0f, true, false);
   _sword_jump_up_r = pool.create("player_jump_up_sword_r", 0.0f, 0.0f, true, false);
   _sword_jump_up_l = pool.create("player_jump_up_sword_l", 0.0f, 0.0f, true, false);
   _sword_jump_midair_r = pool.create("player_jump_midair_sword_r", 0.0f, 0.0f, true, false);
   _sword_jump_midair_l = pool.create("player_jump_midair_sword_l", 0.0f, 0.0f, true, false);
   _sword_jump_down_r = pool.create("player_jump_down_sword_r", 0.0f, 0.0f, true, false);
   _sword_jump_down_l = pool.create("player_jump_down_sword_l", 0.0f, 0.0f, true, false);
   _sword_jump_landing_r = pool.create("player_jump_landing_sword_r", 0.0f, 0.0f, true, false);
   _sword_jump_landing_l = pool.create("player_jump_landing_sword_l", 0.0f, 0.0f, true, false);

   _sword_attack_jump_r = pool.create("player_jump_attack_sword_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_l = pool.create("player_jump_attack_sword_l", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_init_r = pool.create("player_jump_init_attack_sword_legs_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_init_l = pool.create("player_jump_init_attack_sword_legs_l", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_up_r = pool.create("player_jump_up_attack_sword_legs_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_up_l = pool.create("player_jump_up_attack_sword_legs_l", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_midair_r = pool.create("player_jump_midair_attack_sword_legs_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_midair_l = pool.create("player_jump_midair_attack_sword_legs_l", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_down_r = pool.create("player_jump_down_attack_sword_legs_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_down_l = pool.create("player_jump_down_attack_sword_legs_l", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_landing_r = pool.create("player_jump_landing_attack_sword_legs_r", 0.0f, 0.0f, true, false);
   _sword_attack_jump_legs_landing_l = pool.create("player_jump_landing_attack_sword_legs_l", 0.0f, 0.0f, true, false);

   _double_jump_r = pool.create("player_double_jump_r", 0.0f, 0.0f, true, false);
   _double_jump_l = pool.create("player_double_jump_l", 0.0f, 0.0f, true, false);
   _sword_double_jump_r = pool.create("player_double_jump_sword_r", 0.0f, 0.0f, true, false);
   _sword_double_jump_l = pool.create("player_double_jump_sword_l", 0.0f, 0.0f, true, false);

   _swim_r = pool.create("player_swim_r", 0.0f, 0.0f, true, false);
   _swim_l = pool.create("player_swim_l", 0.0f, 0.0f, true, false);
   _sword_swim_r = pool.create("player_swim_sword_r", 0.0f, 0.0f, true, false);
   _sword_swim_l = pool.create("player_swim_sword_l", 0.0f, 0.0f, true, false);

   _wallslide_impact_r = pool.create("player_wallslide_impact_r", 0.0f, 0.0f, true, false);
   _wallslide_impact_l = pool.create("player_wallslide_impact_l", 0.0f, 0.0f, true, false);
   _wallslide_r = pool.create("player_wallslide_r", 0.0f, 0.0f, true, false);
   _wallslide_l = pool.create("player_wallslide_l", 0.0f, 0.0f, true, false);

   _wall_jump_r = pool.create("player_wall_jump_r", 0.0f, 0.0f, true, false);
   _wall_jump_l = pool.create("player_wall_jump_l", 0.0f, 0.0f, true, false);

   _appear_r = pool.create("player_appear_r", 0.0f, 0.0f, true, false);
   _appear_l = pool.create("player_appear_l", 0.0f, 0.0f, true, false);
   _sword_appear_r = pool.create("player_appear_sword_r", 0.0f, 0.0f, true, false);
   _sword_appear_l = pool.create("player_appear_sword_l", 0.0f, 0.0f, true, false);

   _death_default = pool.create("player_death", 0.0f, 0.0f, true, false);
   _death_electrocuted_r = pool.create("player_death_electrocuted_r", 0.0f, 0.0f, true, false);
   _death_electrocuted_l = pool.create("player_death_electrocuted_l", 0.0f, 0.0f, true, false);

   _sword_attack_bend_down_1_l = pool.create("player_bend_down_attack_sword_1_l", 0.0f, 0.0f, true, false);
   _sword_attack_bend_down_1_r = pool.create("player_bend_down_attack_sword_1_r", 0.0f, 0.0f, true, false);
   _sword_attack_bend_down_2_l = pool.create("player_bend_down_attack_sword_2_l", 0.0f, 0.0f, true, false);
   _sword_attack_bend_down_2_r = pool.create("player_bend_down_attack_sword_2_r", 0.0f, 0.0f, true, false);
   _sword_attack_standing_l[0] = pool.create("player_standing_attack_sword_1_l", 0.0f, 0.0f, true, false);
   _sword_attack_standing_r[0] = pool.create("player_standing_attack_sword_1_r", 0.0f, 0.0f, true, false);
   _sword_attack_standing_l[1] = pool.create("player_standing_attack_sword_2_l", 0.0f, 0.0f, true, false);
   _sword_attack_standing_r[1] = pool.create("player_standing_attack_sword_2_r", 0.0f, 0.0f, true, false);
   // _sword_standing_attack_l[2] = pool.add("player_standing_attack_sword_3_l", 0.0f, 0.0f, true, false);
   // _sword_standing_attack_r[2] = pool.add("player_standing_attack_sword_3_r", 0.0f, 0.0f, true, false);

   // _crouch_r = pool.add("player_crouch_r", 0.0f, 0.0f, true, false);
   // _crouch_l = pool.add("player_crouch_l", 0.0f, 0.0f, true, false);

   _wallslide_animation = pool.create("player_wallslide_dust", 0.0f, 0.0f, true, false);

   _appear_animations = {_appear_l, _appear_r, _sword_appear_l, _sword_appear_r};

   // we will replace those later as we go
   _idle_r_tmp = _idle_r;
   _idle_l_tmp = _idle_l;
   _bend_down_idle_r_tmp = _bend_down_idle_r;
   _bend_down_idle_l_tmp = _bend_down_idle_l;
   _sword_attack_standing_tmp_l = _sword_attack_standing_l[0];
   _sword_attack_standing_tmp_r = _sword_attack_standing_r[0];

   // we don't want these to jump back to the first frame
   _appear_r->_reset_to_first_frame = false;
   _appear_l->_reset_to_first_frame = false;
   _sword_appear_r->_reset_to_first_frame = false;
   _sword_appear_l->_reset_to_first_frame = false;

   _death_default->_reset_to_first_frame = false;
   _death_electrocuted_l->_reset_to_first_frame = false;
   _death_electrocuted_r->_reset_to_first_frame = false;

   _bend_down_r->_reset_to_first_frame = false;
   _bend_down_l->_reset_to_first_frame = false;
   _sword_bend_down_r->_reset_to_first_frame = false;
   _sword_bend_down_l->_reset_to_first_frame = false;

   _bend_up_r->_reset_to_first_frame = false;
   _bend_up_l->_reset_to_first_frame = false;
   _sword_bend_up_r->_reset_to_first_frame = false;
   _sword_bend_up_l->_reset_to_first_frame = false;

   _dash_init_r->_reset_to_first_frame = false;
   _dash_init_l->_reset_to_first_frame = false;
   _dash_stop_r->_reset_to_first_frame = false;
   _dash_stop_l->_reset_to_first_frame = false;
   _sword_dash_init_r->_reset_to_first_frame = false;
   _sword_dash_init_l->_reset_to_first_frame = false;
   _sword_dash_stop_r->_reset_to_first_frame = false;
   _sword_dash_stop_l->_reset_to_first_frame = false;

   // we just reverse the bend down animation
   _bend_up_r->reverse();
   _bend_up_l->reverse();
   _bend_up_r->_name = "player_bend_up_r";
   _bend_up_l->_name = "player_bend_up_l";
   _sword_bend_up_r->reverse();
   _sword_bend_up_l->reverse();
   _sword_bend_up_r->_name = "player_bend_up_sword_r";
   _sword_bend_up_l->_name = "player_bend_up_sword_l";

   // dash stop is also just reversed
   _dash_stop_r->reverse();
   _dash_stop_l->reverse();
   _dash_stop_r->_name = "player_dash_stop_r";
   _dash_stop_l->_name = "player_dash_stop_l";

   // fill lut to map sword cycles onto regular move cycles
   _sword_lut[_appear_l] = _sword_appear_l;
   _sword_lut[_appear_r] = _sword_appear_r;
   _sword_lut[_bend_down_idle_blink_l] = _sword_bend_down_idle_blink_l;
   _sword_lut[_bend_down_idle_blink_r] = _sword_bend_down_idle_blink_r;
   _sword_lut[_bend_down_idle_l] = _sword_bend_down_idle_l;
   _sword_lut[_bend_down_idle_r] = _sword_bend_down_idle_r;
   _sword_lut[_bend_down_l] = _sword_bend_down_l;
   _sword_lut[_bend_down_r] = _sword_bend_down_r;
   _sword_lut[_bend_up_l] = _sword_bend_up_l;
   _sword_lut[_bend_up_r] = _sword_bend_up_r;
   _sword_lut[_dash_init_l] = _sword_bend_down_l;
   _sword_lut[_dash_init_r] = _sword_dash_init_r;
   _sword_lut[_dash_l] = _sword_dash_l;
   _sword_lut[_dash_r] = _sword_dash_r;
   _sword_lut[_dash_stop_l] = _sword_dash_stop_l;
   _sword_lut[_dash_stop_r] = _sword_dash_stop_r;
   _sword_lut[_double_jump_l] = _sword_double_jump_l;
   _sword_lut[_double_jump_r] = _sword_double_jump_r;
   _sword_lut[_idle_l] = _sword_idle_l;
   _sword_lut[_idle_r] = _sword_idle_r;
   _sword_lut[_idle_blink_l] = _sword_idle_blink_l;
   _sword_lut[_idle_blink_r] = _sword_idle_blink_r;
   _sword_lut[_run_l] = _sword_run_l;
   _sword_lut[_run_r] = _sword_run_r;
   _sword_lut[_swim_l] = _sword_swim_l;
   _sword_lut[_swim_r] = _sword_swim_r;
   _sword_lut[_jump_init_r] = _sword_jump_init_r;
   _sword_lut[_jump_init_l] = _sword_jump_init_l;
   _sword_lut[_jump_up_r] = _sword_jump_up_r;
   _sword_lut[_jump_up_l] = _sword_jump_up_l;
   _sword_lut[_jump_midair_r] = _sword_jump_midair_r;
   _sword_lut[_jump_midair_l] = _sword_jump_midair_l;
   _sword_lut[_jump_down_r] = _sword_jump_down_r;
   _sword_lut[_jump_down_l] = _sword_jump_down_l;
   _sword_lut[_jump_landing_r] = _sword_jump_landing_r;
   _sword_lut[_jump_landing_l] = _sword_jump_landing_l;

   // map cycles for in-air attack with sword
   _sword_attack_lut[_jump_init_r] = _sword_attack_jump_legs_init_r;
   _sword_attack_lut[_jump_init_l] = _sword_attack_jump_legs_init_l;
   _sword_attack_lut[_jump_up_r] = _sword_attack_jump_legs_up_r;
   _sword_attack_lut[_jump_up_l] = _sword_attack_jump_legs_up_l;
   _sword_attack_lut[_jump_midair_r] = _sword_attack_jump_legs_midair_r;
   _sword_attack_lut[_jump_midair_l] = _sword_attack_jump_legs_midair_l;
   _sword_attack_lut[_jump_down_r] = _sword_attack_jump_legs_down_r;
   _sword_attack_lut[_jump_down_l] = _sword_attack_jump_legs_down_l;
   _sword_attack_lut[_jump_landing_r] = _sword_attack_jump_legs_landing_r;
   _sword_attack_lut[_jump_landing_l] = _sword_attack_jump_legs_landing_l;

   // set up looped animations
   _looped_animations.push_back(_idle_r);
   _looped_animations.push_back(_idle_l);
   _looped_animations.push_back(_idle_blink_r);
   _looped_animations.push_back(_idle_blink_l);
   _looped_animations.push_back(_swim_r);
   _looped_animations.push_back(_swim_l);
   _looped_animations.push_back(_run_r);
   _looped_animations.push_back(_run_l);
   _looped_animations.push_back(_dash_r);
   _looped_animations.push_back(_dash_l);
   _looped_animations.push_back(_dash_init_l);
   _looped_animations.push_back(_dash_init_r);
   _looped_animations.push_back(_dash_stop_r);
   _looped_animations.push_back(_dash_stop_l);
   _looped_animations.push_back(_jump_init_r);
   _looped_animations.push_back(_jump_init_l);
   _looped_animations.push_back(_jump_up_r);
   _looped_animations.push_back(_jump_up_l);
   _looped_animations.push_back(_jump_down_r);
   _looped_animations.push_back(_jump_down_l);
   _looped_animations.push_back(_jump_landing_r);
   _looped_animations.push_back(_jump_landing_l);
   _looped_animations.push_back(_double_jump_r);
   _looped_animations.push_back(_double_jump_l);
   _looped_animations.push_back(_wallslide_impact_r);
   _looped_animations.push_back(_wallslide_impact_l);
   _looped_animations.push_back(_wallslide_r);
   _looped_animations.push_back(_wallslide_l);
   _looped_animations.push_back(_wall_jump_r);
   _looped_animations.push_back(_wall_jump_l);
   _looped_animations.push_back(_wallslide_animation);

   for (auto& loop_animation : _looped_animations)
   {
      loop_animation->_looped = true;

      // also update those in the lut
      const auto sword_animation_it = _sword_attack_lut.find(loop_animation);
      if (sword_animation_it != _sword_attack_lut.end())
      {
         (*sword_animation_it).second->_looped = true;
      }
   }
}

int32_t PlayerAnimation::getJumpAnimationReference() const
{
   return _jump_animation_reference;
}

const std::shared_ptr<Animation>& PlayerAnimation::getCurrentCycle() const
{
   return _current_cycle;
}

void PlayerAnimation::resetAlpha()
{
   // reset alphas if needed
   for (auto& a : _looped_animations)
   {
      a->setAlpha(255);
   }
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processDeathAnimation(const PlayerAnimationData& data)
{
   std::optional<std::shared_ptr<Animation>> next_cycle;
   // death animation
   if (data._dead)
   {
      if (data._death_reason == DeathReason::Laser)
      {
         next_cycle = data._points_right ? _death_electrocuted_r : _death_electrocuted_l;
      }
      else
      {
         next_cycle = _death_default;
      }
   }

   return next_cycle;
}

std::optional<std::shared_ptr<Animation>>
PlayerAnimation::processAttackAnimation(const std::shared_ptr<Animation>& next_cycle, const PlayerAnimationData& data)
{
   std::optional<std::shared_ptr<Animation>> attack_cycle;

   switch (data._weapon_type)
   {
      case WeaponType::Sword:
      {
         const auto in_air_attack_elapsed =
            StopWatch::duration(data._timepoint_attack_jumping_start, now) >= _sword_attack_jump_r->_overall_time_chrono;

         const auto bend_down_attack_elapsed =
            StopWatch::duration(data._timepoint_attack_bend_down_start, now) >= _sword_attack_bend_down_1_l->_overall_time_chrono;

         if (!bend_down_attack_elapsed)
         {
            attack_cycle = data._points_left ? _sword_attack_bend_down_1_l : _sword_attack_bend_down_1_r;
         }

         // only handle in-air attack when the player is actually in the air, otherwise just
         // cancel the animation entirely; this allows to play a standing attack animation
         // right after landing
         else if (!in_air_attack_elapsed && data._in_air)
         {
            // scenario 1: jump ends before attack ends
            //
            // |
            // |                [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
            // |[ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
            // +------------------------------------------------------------->
            // |                |                  |
            // jump start       |                  |
            //                  attack start       |
            //                                     abort attack
            //
            // scenario 2: jump ends after attack ends
            //
            // |
            // |            [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][x]
            // |         [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
            // +------------------------------------------------------------->
            //           |                |               |
            //           jump start       |               |
            //                            attack start    |
            //                                            play regular jump animation again

            // for in-air, just replace the jump cycles with the cycles that just have pants :)
            const auto sword_attack_cycle_it = _sword_attack_lut.find(next_cycle);
            if (sword_attack_cycle_it != _sword_attack_lut.end())
            {
               _auxiliary_cycle = data._points_left ? _sword_attack_jump_l : _sword_attack_jump_r;
               return sword_attack_cycle_it->second;
            }

            // jump animation is over, clear auxiliary cycle
            //
            // this is not a solid implementation since the attack cycle should be played until it's
            // actually over... but for that, more jump/idle cycles, just showing the pants are needed.
            _auxiliary_cycle = nullptr;
         }
         else
         {
            const auto standing_attack_elapsed = StopWatch::duration(data._timepoint_attack_standing_start, now) >=
                                                 (data._points_left ? _sword_attack_standing_tmp_l->_overall_time_chrono
                                                                    : _sword_attack_standing_tmp_r->_overall_time_chrono);

            if (!standing_attack_elapsed)
            {
               if (data._points_left)
               {
                  attack_cycle = _sword_attack_standing_tmp_l;
                  _sword_attack_standing_l_reset = true;
               }
               else
               {
                  attack_cycle = _sword_attack_standing_tmp_r;
                  _sword_attack_standing_r_reset = true;
               }
            }
            else
            {
               _sword_attack_standing_tmp_l->_finished = true;
               _sword_attack_standing_tmp_r->_finished = true;
            }

            // pick a different attack cycle
            if (_sword_attack_standing_l_reset && _sword_attack_standing_tmp_l->_finished)
            {
               _sword_attack_standing_tmp_l = _sword_attack_standing_l[(std::rand() % _sword_attack_standing_l.size())];
               _sword_attack_standing_l_reset = false;
            }

            if (_sword_attack_standing_r_reset && _sword_attack_standing_tmp_r->_finished)
            {
               _sword_attack_standing_tmp_r = _sword_attack_standing_r[(std::rand() % _sword_attack_standing_r.size())];
               _sword_attack_standing_r_reset = false;
            }
         }

         // if the in-air attack is cycle is elapsed or the player landed on the ground, clear auxiliary cycle
         if (in_air_attack_elapsed || !data._in_air)
         {
            _auxiliary_cycle = nullptr;

            // reset sword attack animation once it's elapsed
            _sword_attack_jump_l->pause();
            _sword_attack_jump_r->pause();
            _sword_attack_jump_l->seekToStart();
            _sword_attack_jump_r->seekToStart();
         }

         break;
      }
      case WeaponType::Bow:
      case WeaponType::Gun:
      case WeaponType::None:
      {
         break;
      }
   }

   return attack_cycle;
}

const std::shared_ptr<Animation>& PlayerAnimation::getAuxiliaryCycle() const
{
   return _auxiliary_cycle;
}

const std::shared_ptr<Animation>& PlayerAnimation::getWallslideAnimation() const
{
   return _wallslide_animation;
}

PlayerAnimation::HighResDuration PlayerAnimation::getCurrentAnimationDuration() const
{
   return _current_cycle->_overall_time_chrono;
}

PlayerAnimation::HighResDuration PlayerAnimation::getRevealDuration() const
{
   using namespace std::chrono_literals;
   return 1000ms + _appear_l->_overall_time_chrono + 20ms;
}

PlayerAnimation::HighResDuration PlayerAnimation::getSwordAttackDurationStanding() const
{
   return _sword_attack_standing_l[0]->_overall_time_chrono;
}

PlayerAnimation::HighResDuration PlayerAnimation::getSwordAttackDurationStandingMax() const
{
   return std::ranges::max(
      _sword_attack_standing_l | std::views::transform([](const auto& animation) { return animation->_overall_time_chrono; })
   );
}

PlayerAnimation::HighResDuration PlayerAnimation::getSwordAttackDurationBendingDown1() const
{
   return _sword_attack_bend_down_1_l->_overall_time_chrono;
}

PlayerAnimation::HighResDuration PlayerAnimation::getSwordAttackDurationBendingDown2() const
{
   return _sword_attack_bend_down_2_l->_overall_time_chrono;
}

PlayerAnimation::HighResDuration PlayerAnimation::getSwordAttackDurationJumping() const
{
   return _sword_attack_jump_l->_overall_time_chrono;
}

std::optional<PlayerAnimation::HighResDuration> PlayerAnimation::getActiveAttackCycleDuration()
{
   if (_current_cycle == _sword_attack_bend_down_1_l || _current_cycle == _sword_attack_bend_down_1_r)
   {
      return _sword_attack_bend_down_1_l->_overall_time_chrono;
   }

   if (_current_cycle == _sword_attack_bend_down_2_l || _current_cycle == _sword_attack_bend_down_2_r)
   {
      return _sword_attack_bend_down_2_l->_overall_time_chrono;
   }

   if (_current_cycle == _sword_attack_standing_l[0] || _current_cycle == _sword_attack_standing_r[0])
   {
      return _sword_attack_standing_l[0]->_overall_time_chrono;
   }

   if (_current_cycle == _sword_attack_standing_l[1] || _current_cycle == _sword_attack_standing_r[1])
   {
      return _sword_attack_standing_l[1]->_overall_time_chrono;
   }

   return std::nullopt;
}

const std::shared_ptr<Animation>&
PlayerAnimation::getMappedArmedAnimation(const std::shared_ptr<Animation>& animation, const PlayerAnimationData& animation_data) const
{
   switch (animation_data._weapon_type)
   {
      case WeaponType::None:
      {
         return animation;
      }

      case WeaponType::Sword:
      {
         const auto sword_cycle_it = _sword_lut.find(animation);
         if (sword_cycle_it != _sword_lut.end())
         {
            return sword_cycle_it->second;
         }
         break;
      }
      case WeaponType::Bow:
      case WeaponType::Gun:
      {
         break;
      }
   }

   return animation;
}

bool PlayerAnimation::isBendingUp(const PlayerAnimationData& data) const
{
   const auto& mapped_animation = getMappedArmedAnimation(_bend_up_l, data);
   return (StopWatch::duration(data._timepoint_bend_down_end, now) < mapped_animation->_overall_time_chrono);
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processIdleAnimation(const PlayerAnimationData& data)
{
   const auto look_active = DisplayMode::getInstance().isSet(Display::CameraPanorama);
   const auto move_active = (data._moving_left || data._moving_right) && !look_active;

   if (data._in_water || data._dash_dir.has_value() || data._bending_down || move_active)
   {
      return std::nullopt;
   }

   if (isBendingUp(data))
   {
      return std::nullopt;
   }

   if (data._points_left)
   {
      auto next_cycle = _idle_l_tmp;
      if (getMappedArmedAnimation(_idle_l_tmp, data)->_finished)
      {
         _idle_l_tmp = (std::rand() % 10 == 0) ? _idle_blink_l : _idle_l;
      }

      return next_cycle;
   }

   auto next_cycle = _idle_r_tmp;
   if (getMappedArmedAnimation(_idle_r_tmp, data)->_finished)
   {
      _idle_r_tmp = (std::rand() % 10 == 0) ? _idle_blink_r : _idle_r;
   }

   return next_cycle;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processBendUpAnimation(const PlayerAnimationData& data)
{
   if (!data._bending_down)
   {
      const auto next_cycle = data._points_left ? _bend_up_l : _bend_up_r;
      const auto& mapped_animation = getMappedArmedAnimation(next_cycle, data);

      // bend up if player is releasing the crouch
      if (StopWatch::duration(data._timepoint_bend_down_end, now) < mapped_animation->_overall_time_chrono)
      {
         return next_cycle;
      }
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processBendDownAnimation(const PlayerAnimationData& data)
{
   // bending down state
   if (!data._bending_down)
   {
      return std::nullopt;
   }

   auto next_cycle = data._points_left ? _bend_down_l : _bend_down_r;
   const auto& mapped_animation = getMappedArmedAnimation(next_cycle, data);

   // going from bending down to bending down idle
   if (StopWatch::duration(data._timepoint_bend_down_start, now) > mapped_animation->_overall_time_chrono)
   {
      next_cycle = data._points_left ? _bend_down_idle_l_tmp : _bend_down_idle_r_tmp;

      // blink every now and then
      // it's important to check whether either the normal or the armed cycle is finished
      if (getMappedArmedAnimation(_bend_down_idle_l_tmp, data)->_finished)
      {
         _bend_down_idle_l_tmp = (std::rand() % 100 == 0) ? _bend_down_idle_blink_l : _bend_down_idle_l;
      }

      if (getMappedArmedAnimation(_bend_down_idle_r_tmp, data)->_finished)
      {
         _bend_down_idle_r_tmp = (std::rand() % 100 == 0) ? _bend_down_idle_blink_r : _bend_down_idle_r;
      }
   }

   return next_cycle;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processCrouchAnimation(const PlayerAnimationData& data)
{
   const auto passes_sanity_check = !(data._moving_right && data._moving_left);
   const auto look_active = DisplayMode::getInstance().isSet(Display::CameraPanorama);

   // crouch
   if (data._moving_right && passes_sanity_check && !data._in_air && !data._in_water && !look_active && data._crouching)
   {
      // unsupported
      // next_cycle = _crouch_r;
   }
   else if (data._moving_left && passes_sanity_check && !data._in_air && !data._in_water && !look_active && data._crouching)
   {
      // unsupported
      // next_cycle = _crouch_l;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processRunAnimation(const PlayerAnimationData& data)
{
   const auto passes_sanity_check = !(data._moving_right && data._moving_left);
   const auto look_active = DisplayMode::getInstance().isSet(Display::CameraPanorama);

   // run
   if (!data._dash_dir.has_value()  //
       && passes_sanity_check       //
       && !data._in_air             //
       && !data._in_water           //
       && !look_active              //
       && !data._crouching          //
       && !data._bending_down)
   {
      return data._moving_right ? _run_r : _run_l;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processDashAnimation(const PlayerAnimationData& data)
{
   // dash
   if (!data._dash_dir.has_value())
   {
      return std::nullopt;
   }

   // init  regular            stop
   // |     |                  |
   // +-----+------------------+----->
   // t
   const auto dash_count_max = PhysicsConfiguration::getInstance()._player_dash_frame_count;
   const auto dash_count_regular = dash_count_max - (dash_count_max / 5);
   const auto dash_count_stop = dash_count_max / 5;

   if (data._dash_frame_count > dash_count_regular)
   {
      return (data._dash_dir == Dash::Left) ? _dash_init_l : _dash_init_r;
   }

   if (data._dash_frame_count < dash_count_stop)
   {
      return (data._dash_dir == Dash::Left) ? _dash_stop_l : _dash_stop_r;
   }

   return (data._dash_dir == Dash::Left) ? _dash_l : _dash_r;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processSwimAnimation(const PlayerAnimationData& data)
{
   if (!data._in_water)
   {
      return std::nullopt;
   }

   return data._points_right ? _swim_r : _swim_l;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processWallSlideAnimation(const PlayerAnimationData& data)
{
   if (!data._wall_sliding)
   {
      return std::nullopt;
   }

   if (data._dash_dir.has_value())
   {
      return std::nullopt;
   }

   const auto& mapped_animation = getMappedArmedAnimation(_wallslide_impact_l, data);

   if (StopWatch::duration(data._timepoint_wallslide, now) < mapped_animation->_overall_time_chrono)
   {
      return data._points_right ? _wallslide_impact_l : _wallslide_impact_r;
   }

   return data._points_right ? _wallslide_l : _wallslide_r;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processWallJumpAnimation(const PlayerAnimationData& data)
{
   if (StopWatch::duration(data._timepoint_walljump, now) < getMappedArmedAnimation(_wall_jump_r, data)->_overall_time_chrono)
   {
      return data._wall_jump_points_right ? _wall_jump_r : _wall_jump_l;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processDoubleJumpAnimation(const PlayerAnimationData& data)
{
   if (StopWatch::duration(data._timepoint_doublejump, now) < getMappedArmedAnimation(_double_jump_r, data)->_overall_time_chrono)
   {
      return data._points_right ? _double_jump_r : _double_jump_l;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processScreenTransitionIdleAnimation(const PlayerAnimationData& data)
{
   // force idle for screen transitions
   if (DisplayMode::getInstance().isSet(Display::ScreenTransition))
   {
      if (data._in_water)
      {
         return data._points_left ? _swim_l : _swim_r;
      }

      return data._points_left ? _idle_l_tmp : _idle_r_tmp;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processJumpAnimation(const PlayerAnimationData& data)
{
   // jump init
   if (data._dash_dir.has_value())
   {
      return std::nullopt;
   }

   // jump ignition
   if (data._jump_frame_count > PhysicsConfiguration::getInstance()._player_jump_frame_count - FRAMES_COUNT_JUMP_INIT)
   {
      _jump_animation_reference = 0;
      return data._points_right ? _jump_init_r : _jump_init_l;
   }

   // jump is active when either
   // - in the air
   // - jumping through a one-sided wall (in that case player may have ground contacts)
   if ((data._in_air || data._jumping_through_one_way_wall) && !data._in_water)
   {
      const auto velocity = data._linear_velocity;

      // jump movement goes up
      if (velocity.y < JUMP_UP_VELOCITY_THRESHOLD)
      {
         _jump_animation_reference = 1;
         return data._points_right ? _jump_up_r : _jump_up_l;
      }

      // jump movement goes down
      if (velocity.y > JUMP_DOWN_VELOCITY_THRESHOLD)
      {
         _jump_animation_reference = 2;
         return data._points_right ? _jump_down_r : _jump_down_l;
      }

      // jump midair
      if (_jump_animation_reference == 1)
      {
         return data._points_right ? _jump_midair_r : _jump_midair_l;
      }

      // still in-air but linear velocity is ~0 for whatever reason
      if (_jump_animation_reference == 2)
      {
         return data._points_right ? _jump_down_r : _jump_down_l;
      }
   }

   // hard landing
   else if (_jump_animation_reference == 2 && data._hard_landing)
   {
      std::optional<std::shared_ptr<Animation>> next_cycle;
      next_cycle = data._points_right ? _jump_landing_r : _jump_landing_l;

      if (next_cycle.value()->_current_frame == static_cast<int32_t>(next_cycle.value()->_frames.size()) - 1)
      {
         _jump_animation_reference = 3;
         next_cycle.value()->seekToStart();
      }

      return next_cycle;
   }

   return std::nullopt;
}

std::optional<std::shared_ptr<Animation>> PlayerAnimation::processAppearAnimation(const PlayerAnimationData& data)
{
   // if player didn't die earlier and is also located at the start position, don't play the appear animation
   if (data._death_count_current_level == 0 && data._checkpoint_index == 0)
   {
      return std::nullopt;
   }

   using namespace std::chrono_literals;

   std::optional<std::shared_ptr<Animation>> next_cycle;

   // appear animation is split into two stages
   // 1) just waiting for a second until all camera focusing has been completed
   // 2) playing the appear animation
   if (GameClock::getInstance().durationSinceSpawn() < getRevealDuration())
   {
      next_cycle = data._points_right ? _appear_r : _appear_l;

      if (GameClock::getInstance().durationSinceSpawn() < 1.0s)
      {
         // invisibility: 0 .. 1.0s (wait until player is focused)
         for (auto& appear_animation : _appear_animations)
         {
            appear_animation->seekToStart();
            appear_animation->setVisible(false);
         }
      }
      else
      {
         // player appear animation for 20 x 20ms, plus an extra frame - just to be sure :)
         for (auto& appear_animation : _appear_animations)
         {
            appear_animation->play();
            appear_animation->setVisible(true);
         }
      }
   }

   return next_cycle;
}

void PlayerAnimation::update(const sf::Time& dt, const PlayerAnimationData& data)
{
   using namespace std::chrono_literals;

   if (Portal::isLocked())
   {
      return;
   }

   now = StopWatch::now();

   std::shared_ptr<Animation> next_cycle;
   next_cycle = processDashAnimation(data).value_or(next_cycle);
   next_cycle = processRunAnimation(data).value_or(next_cycle);
   next_cycle = processCrouchAnimation(data).value_or(next_cycle);
   next_cycle = processBendDownAnimation(data).value_or(next_cycle);
   next_cycle = processBendUpAnimation(data).value_or(next_cycle);
   next_cycle = processIdleAnimation(data).value_or(next_cycle);
   next_cycle = processJumpAnimation(data).value_or(next_cycle);
   next_cycle = processSwimAnimation(data).value_or(next_cycle);
   next_cycle = processWallSlideAnimation(data).value_or(next_cycle);
   next_cycle = processDoubleJumpAnimation(data).value_or(next_cycle);
   next_cycle = processWallJumpAnimation(data).value_or(next_cycle);
   next_cycle = processScreenTransitionIdleAnimation(data).value_or(next_cycle);
   next_cycle = processAppearAnimation(data).value_or(next_cycle);
   next_cycle = processDeathAnimation(data).value_or(next_cycle);
   next_cycle = processAttackAnimation(next_cycle, data).value_or(next_cycle);

   next_cycle = getMappedArmedAnimation(next_cycle, data);

   if (!next_cycle)
   {
      Log::Error() << "invalid animation cycle chosen";
      return;
   }

   // reset x if animation cycle changed
   if (next_cycle != _current_cycle)
   {
      // std::cout << next_cycle->_name << std::endl;

      next_cycle->seekToStart();
      next_cycle->play();
   }

   // i keep this here
   // might not be the last time to debug an impossible sequence of animation cycles
   //
   //   if (_current_cycle && next_cycle && _current_cycle->_name == "player_idle_blink_r" && next_cycle->_name == "player_jump_down_r")
   //   {
   //      std::cout << "we're fucked." << std::endl;
   //      std::cout << data << std::endl;
   //   }

   _current_cycle = next_cycle;
   _current_cycle->update(dt);

   if (_auxiliary_cycle)
   {
      _auxiliary_cycle->play();
      _auxiliary_cycle->update(dt);
   }
}
