
#include "playeranimation.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#include "animationpool.h"
#include "camerapane.h"
#include "displaymode.h"
#include "framework/tools/log.h"
#include "framework/tools/stopwatch.h"
#include "game/gameclock.h"
#include "mechanisms/portal.h"
#include "physics/physicsconfiguration.h"

constexpr auto FRAMES_COUNT_JUMP_INIT = 3;
constexpr auto JUMP_UP_VELOCITY_THRESHOLD = -1.2f;
constexpr auto JUMP_DOWN_VELOCITY_THRESHOLD = 1.2f;


PlayerAnimation::PlayerAnimation()
{
   // none of the player animations are managed by the animation pool, they're just paused when finished
   _idle_r        = AnimationPool::getInstance().add("player_idle_r",         0.0f, 0.0f, true, false);
   _idle_l        = AnimationPool::getInstance().add("player_idle_l",         0.0f, 0.0f, true, false);
   _swim_r        = AnimationPool::getInstance().add("player_swim_r",         0.0f, 0.0f, true, false);
   _swim_l        = AnimationPool::getInstance().add("player_swim_l",         0.0f, 0.0f, true, false);
   _run_r         = AnimationPool::getInstance().add("player_run_r",          0.0f, 0.0f, true, false);
   _run_l         = AnimationPool::getInstance().add("player_run_l",          0.0f, 0.0f, true, false);
   _dash_r        = AnimationPool::getInstance().add("player_dash_r",         0.0f, 0.0f, true, false);
   _dash_l        = AnimationPool::getInstance().add("player_dash_l",         0.0f, 0.0f, true, false);
   _crouch_r      = AnimationPool::getInstance().add("player_crouch_r",       0.0f, 0.0f, true, false);
   _crouch_l      = AnimationPool::getInstance().add("player_crouch_l",       0.0f, 0.0f, true, false);

   _jump_init_r    = AnimationPool::getInstance().add("player_jump_init_r",    0.0f, 0.0f, true, false);
   _jump_up_r      = AnimationPool::getInstance().add("player_jump_up_r",      0.0f, 0.0f, true, false);
   _jump_midair_r  = AnimationPool::getInstance().add("player_jump_midair_r",  0.0f, 0.0f, true, false);
   _jump_down_r    = AnimationPool::getInstance().add("player_jump_down_r",    0.0f, 0.0f, true, false);
   _jump_landing_r = AnimationPool::getInstance().add("player_jump_landing_r", 0.0f, 0.0f, true, false);

   _jump_init_l    = AnimationPool::getInstance().add("player_jump_init_l",    0.0f, 0.0f, true, false);
   _jump_up_l      = AnimationPool::getInstance().add("player_jump_up_l",      0.0f, 0.0f, true, false);
   _jump_midair_l  = AnimationPool::getInstance().add("player_jump_midair_l",  0.0f, 0.0f, true, false);
   _jump_down_l    = AnimationPool::getInstance().add("player_jump_down_l",    0.0f, 0.0f, true, false);
   _jump_landing_l = AnimationPool::getInstance().add("player_jump_landing_l", 0.0f, 0.0f, true, false);

   // version 2
   _idle_r_2             = AnimationPool::getInstance().add("player_idle_r_2",             0.0f, 0.0f, true, false);
   _idle_l_2             = AnimationPool::getInstance().add("player_idle_l_2",             0.0f, 0.0f, true, false);
   _idle_blink_r_2       = AnimationPool::getInstance().add("player_idle_blink_r_2",       0.0f, 0.0f, true, false);
   _idle_blink_l_2       = AnimationPool::getInstance().add("player_idle_blink_l_2",       0.0f, 0.0f, true, false);
   _bend_down_r_2        = AnimationPool::getInstance().add("player_bend_down_r_2",        0.0f, 0.0f, true, false);
   _bend_down_l_2        = AnimationPool::getInstance().add("player_bend_down_l_2",        0.0f, 0.0f, true, false);
   _bend_up_r_2          = AnimationPool::getInstance().add("player_bend_down_r_2",        0.0f, 0.0f, true, false);
   _bend_up_l_2          = AnimationPool::getInstance().add("player_bend_down_l_2",        0.0f, 0.0f, true, false);
   _idle_to_run_r_2      = AnimationPool::getInstance().add("player_idle_to_run_r_2",      0.0f, 0.0f, true, false);
   _idle_to_run_l_2      = AnimationPool::getInstance().add("player_idle_to_run_l_2",      0.0f, 0.0f, true, false);
   _runstop_r_2          = AnimationPool::getInstance().add("player_runstop_r_2",          0.0f, 0.0f, true, false);
   _runstop_l_2          = AnimationPool::getInstance().add("player_runstop_l_2",          0.0f, 0.0f, true, false);
   _run_r_2              = AnimationPool::getInstance().add("player_run_r_2",              0.0f, 0.0f, true, false);
   _run_l_2              = AnimationPool::getInstance().add("player_run_l_2",              0.0f, 0.0f, true, false);

   _dash_init_r_2        = AnimationPool::getInstance().add("player_dash_init_r_2",        0.0f, 0.0f, true, false);
   _dash_init_l_2        = AnimationPool::getInstance().add("player_dash_init_l_2",        0.0f, 0.0f, true, false);
   _dash_r_2             = AnimationPool::getInstance().add("player_dash_r_2",             0.0f, 0.0f, true, false);
   _dash_l_2             = AnimationPool::getInstance().add("player_dash_l_2",             0.0f, 0.0f, true, false);
   _dash_stop_r_2        = AnimationPool::getInstance().add("player_dash_init_r_2",        0.0f, 0.0f, true, false);
   _dash_stop_l_2        = AnimationPool::getInstance().add("player_dash_init_l_2",        0.0f, 0.0f, true, false);

   // _crouch_r_2           = AnimationPool::getInstance().add("player_crouch_r_2",           0.0f, 0.0f, true, false);
   // _crouch_l_2           = AnimationPool::getInstance().add("player_crouch_l_2",           0.0f, 0.0f, true, false);

   _jump_init_r_2        = AnimationPool::getInstance().add("player_jump_init_r_2",        0.0f, 0.0f, true, false);
   _jump_up_r_2          = AnimationPool::getInstance().add("player_jump_up_r_2",          0.0f, 0.0f, true, false);
   _jump_midair_r_2      = AnimationPool::getInstance().add("player_jump_midair_r_2",      0.0f, 0.0f, true, false);
   _jump_down_r_2        = AnimationPool::getInstance().add("player_jump_down_r_2",        0.0f, 0.0f, true, false);
   _jump_landing_r_2     = AnimationPool::getInstance().add("player_jump_landing_r_2",     0.0f, 0.0f, true, false);

   _jump_init_l_2        = AnimationPool::getInstance().add("player_jump_init_l_2",        0.0f, 0.0f, true, false);
   _jump_up_l_2          = AnimationPool::getInstance().add("player_jump_up_l_2",          0.0f, 0.0f, true, false);
   _jump_midair_l_2      = AnimationPool::getInstance().add("player_jump_midair_l_2",      0.0f, 0.0f, true, false);
   _jump_down_l_2        = AnimationPool::getInstance().add("player_jump_down_l_2",        0.0f, 0.0f, true, false);
   _jump_landing_l_2     = AnimationPool::getInstance().add("player_jump_landing_l_2",     0.0f, 0.0f, true, false);

   _double_jump_r_2      = AnimationPool::getInstance().add("player_double_jump_r_2",      0.0f, 0.0f, true, false);
   _double_jump_l_2      = AnimationPool::getInstance().add("player_double_jump_l_2",      0.0f, 0.0f, true, false);
   _swim_idle_r_2        = AnimationPool::getInstance().add("player_swim_idle_r_2",        0.0f, 0.0f, true, false);
   _swim_idle_l_2        = AnimationPool::getInstance().add("player_swim_idle_l_2",        0.0f, 0.0f, true, false);
   _swim_r_2             = AnimationPool::getInstance().add("player_swim_r_2",             0.0f, 0.0f, true, false);
   _swim_l_2             = AnimationPool::getInstance().add("player_swim_l_2",             0.0f, 0.0f, true, false);

   _wallslide_impact_r_2 = AnimationPool::getInstance().add("player_wallslide_impact_r_2", 0.0f, 0.0f, true, false);
   _wallslide_impact_l_2 = AnimationPool::getInstance().add("player_wallslide_impact_l_2", 0.0f, 0.0f, true, false);
   _wallslide_r_2        = AnimationPool::getInstance().add("player_wallslide_r_2",        0.0f, 0.0f, true, false);
   _wallslide_l_2        = AnimationPool::getInstance().add("player_wallslide_l_2",        0.0f, 0.0f, true, false);
   _wall_jump_r_2        = AnimationPool::getInstance().add("player_wall_jump_r_2",        0.0f, 0.0f, true, false);
   _wall_jump_l_2        = AnimationPool::getInstance().add("player_wall_jump_l_2",        0.0f, 0.0f, true, false);
   _appear_r_2           = AnimationPool::getInstance().add("player_appear_r_2",           0.0f, 0.0f, true, false);
   _appear_l_2           = AnimationPool::getInstance().add("player_appear_l_2",           0.0f, 0.0f, true, false);

   _bend_down_idle_r_2       = AnimationPool::getInstance().add("player_bend_down_idle_r_2", 0.0f, 0.0f, true, false);
   _bend_down_idle_l_2       = AnimationPool::getInstance().add("player_bend_down_idle_l_2", 0.0f, 0.0f, true, false);
   _bend_down_idle_blink_r_2 = AnimationPool::getInstance().add("player_bend_down_idle_blink_r_2", 0.0f, 0.0f, true, false);
   _bend_down_idle_blink_l_2 = AnimationPool::getInstance().add("player_bend_down_idle_blink_l_2", 0.0f, 0.0f, true, false);

   // we will replace those later as we go
   _idle_r_tmp = _idle_r_2;
   _idle_l_tmp = _idle_l_2;
   _bend_down_idle_r_tmp = _bend_down_idle_r_2;
   _bend_down_idle_l_tmp = _bend_down_idle_l_2;

   // we don't want these to jump back to the first frame
   _appear_r_2->_reset_to_first_frame = false;
   _appear_l_2->_reset_to_first_frame = false;
   _bend_down_r_2->_reset_to_first_frame = false;
   _bend_down_l_2->_reset_to_first_frame = false;
   _bend_up_r_2->_reset_to_first_frame = false;
   _bend_up_l_2->_reset_to_first_frame = false;
   _dash_init_r_2->_reset_to_first_frame = false;
   _dash_init_l_2->_reset_to_first_frame = false;
   _dash_stop_r_2->_reset_to_first_frame = false;
   _dash_stop_l_2->_reset_to_first_frame = false;

   // we just reverse the bend down animation
   _bend_up_r_2->reverse();
   _bend_up_l_2->reverse();
   _bend_up_r_2->_name = "player_bend_up_r_2";
   _bend_up_l_2->_name = "player_bend_up_l_2";

   // dash stop is also just reversed
   _dash_stop_r_2->reverse();
   _dash_stop_l_2->reverse();
   _dash_stop_r_2->_name = "player_dash_stop_r_2";
   _dash_stop_l_2->_name = "player_dash_stop_l_2";

   // store all
   _looped_animations.push_back(_idle_r);
   _looped_animations.push_back(_idle_l);
   _looped_animations.push_back(_swim_r);
   _looped_animations.push_back(_swim_l);
   _looped_animations.push_back(_run_r);
   _looped_animations.push_back(_run_l);
   _looped_animations.push_back(_dash_r);
   _looped_animations.push_back(_dash_l);
   _looped_animations.push_back(_crouch_r);
   _looped_animations.push_back(_crouch_l);

   _looped_animations.push_back(_jump_init_r);
   _looped_animations.push_back(_jump_up_r);
   _looped_animations.push_back(_jump_down_r);
   _looped_animations.push_back(_jump_landing_r);
   _looped_animations.push_back(_jump_midair_r);

   _looped_animations.push_back(_jump_init_l);
   _looped_animations.push_back(_jump_up_l);
   _looped_animations.push_back(_jump_down_l);
   _looped_animations.push_back(_jump_landing_l);
   _looped_animations.push_back(_jump_midair_l);

   // version 2
   _looped_animations.push_back(_idle_r_2);
   _looped_animations.push_back(_idle_l_2);
   _looped_animations.push_back(_idle_blink_r_2);
   _looped_animations.push_back(_idle_blink_l_2);
   _looped_animations.push_back(_swim_r_2);
   _looped_animations.push_back(_swim_l_2);
   _looped_animations.push_back(_run_r_2);
   _looped_animations.push_back(_run_l_2);
   _looped_animations.push_back(_dash_r_2);
   _looped_animations.push_back(_dash_init_l_2);
   _looped_animations.push_back(_dash_init_r_2);
   _looped_animations.push_back(_dash_l_2);
   _looped_animations.push_back(_dash_stop_r_2);
   _looped_animations.push_back(_dash_stop_l_2);

   _looped_animations.push_back(_jump_init_r_2);
   _looped_animations.push_back(_jump_up_r_2);
   _looped_animations.push_back(_jump_down_r_2);
   _looped_animations.push_back(_jump_landing_r_2);
   _looped_animations.push_back(_jump_midair_r_2);

   _looped_animations.push_back(_jump_init_l_2);
   _looped_animations.push_back(_jump_up_l_2);
   _looped_animations.push_back(_jump_down_l_2);
   _looped_animations.push_back(_jump_landing_l_2);
   _looped_animations.push_back(_jump_midair_l_2);

   _looped_animations.push_back(_double_jump_r_2);
   _looped_animations.push_back(_double_jump_l_2);
   _looped_animations.push_back(_swim_idle_r_2);
   _looped_animations.push_back(_swim_idle_l_2);
   _looped_animations.push_back(_swim_r_2);
   _looped_animations.push_back(_swim_l_2);

   _looped_animations.push_back(_wallslide_impact_r_2);
   _looped_animations.push_back(_wallslide_impact_l_2);
   _looped_animations.push_back(_wallslide_r_2);
   _looped_animations.push_back(_wallslide_l_2);
   _looped_animations.push_back(_wall_jump_r_2);
   _looped_animations.push_back(_wall_jump_l_2);


   for (auto& i : _looped_animations)
   {
      i->_looped = true;
   }
}


void PlayerAnimation::update(
   const sf::Time& dt,
   const PlayerAnimationData& data
)
{
   if (_version == Version::V1)
   {
      updateV1(dt, data);
   }
   else
   {
      updateV2(dt, data);
   }
}


int32_t PlayerAnimation::getJumpAnimationReference() const
{
   return _jump_animation_reference;
}


std::shared_ptr<Animation> PlayerAnimation::getCurrentCycle() const
{
   return _current_cycle;
}


void PlayerAnimation::resetAlpha()
{
   // reset alphas if needed
   for (auto& a: _looped_animations)
   {
      a->setAlpha(255);
   }
}


void PlayerAnimation::generateJson()
{
   // 00 - player_idle_r, 8
   // 01 - player_idle_l, 8
   // 02 - player_bend_down_r, 8
   // 03 - player_bend_down_l, 8
   // 04 - player_idle_to_run_r, 2
   // 05 - player_idle_to_run_l, 2
   // 06 - player_runstop_r, 0
   // 07 - player_runstop_l, 0
   // 08 - player_run_r, 12
   // 09 - player_run_l, 12
   // 10 - player_dash_r, 5
   // 11 - player_dash_l, 5
   // 12 - player_jump_r, 0
   // 13 - player_jump_l, 0
   // 14 - player_double_jump_r, 0
   // 15 - player_double_jump_l, 0
   // 16 - player_swim_idle_r, 12
   // 17 - player_swim_idle_l, 12
   // 18 - player_swim_r, 0
   // 19 - player_swim_l, 0
   // 20 - player_wallslide_r, 6
   // 21 - player_wallslide_l, 6
   // 22 - player_wall_jump_r, 0
   // 23 - player_wall_jump_l, 0
   // 24 - player_appear_r, 12
   // 25 - player_appear_l, 12

   const auto d_40 = sf::seconds(0.040f);
   const auto d_60 = sf::seconds(0.060f);
   const auto d_75 = sf::seconds(0.075f);
   const auto d_120 = sf::seconds(0.120f);

   const auto sprite_name = "data/sprites/player_unarmed.png";
   auto row = 0;

   const auto next_row = [&](){return (row++) * PIXELS_PER_TILE * 2;};
   const auto col = [](int32_t x){return PIXELS_PER_TILE * 3 * x;};
   const auto v = [d_75](int32_t size){std::vector<sf::Time> arr; for (auto i = 0; i < size; i++) arr.push_back(d_75); return arr;};
   const auto vx = [](int32_t size, const sf::Time& t){std::vector<sf::Time> arr; for (auto i = 0; i < size; i++) arr.push_back(t); return arr;};

   const auto idle_row_r = next_row();
   const auto idle_row_l = next_row();
   AnimationSettings player_idle_r({72, 48}, {0, idle_row_r}, {36.0, 48.0}, vx(8, d_120), sprite_name);
   AnimationSettings player_idle_l({72, 48}, {0, idle_row_l}, {36.0, 48.0}, vx(8, d_120), sprite_name);
   AnimationSettings player_idle_blink_r({72, 48}, {col(8), idle_row_r}, {36.0, 48.0}, vx(8, d_120), sprite_name);
   AnimationSettings player_idle_blink_l({72, 48}, {col(8), idle_row_l}, {36.0, 48.0}, vx(8, d_120), sprite_name);

   const auto bend_down_row_r = next_row();
   const auto bend_down_row_l = next_row();
   AnimationSettings player_bend_down_r({72, 48}, {0, bend_down_row_r}, {36.0, 48.0}, vx(7, d_40), sprite_name);
   AnimationSettings player_bend_down_l({72, 48}, {0, bend_down_row_l}, {36.0, 48.0}, vx(7, d_40), sprite_name);
   AnimationSettings player_bend_down_idle_r({72, 48}, {col(6), bend_down_row_r}, {36.0, 48.0}, vx(1, d_40), sprite_name);
   AnimationSettings player_bend_down_idle_l({72, 48}, {col(6), bend_down_row_l}, {36.0, 48.0}, vx(1, d_40), sprite_name);
   AnimationSettings player_bend_down_idle_blink_r({72, 48}, {col(6), bend_down_row_r}, {36.0, 48.0}, vx(4, d_40), sprite_name);
   AnimationSettings player_bend_down_idle_blink_l({72, 48}, {col(6), bend_down_row_l}, {36.0, 48.0}, vx(4, d_40), sprite_name);

   AnimationSettings player_idle_to_run_r({72, 48}, {0, next_row()}, {36.0, 48.0}, {d_75, d_75}, sprite_name);
   AnimationSettings player_idle_to_run_l({72, 48}, {0, next_row()}, {36.0, 48.0}, {d_75, d_75}, sprite_name);

   AnimationSettings player_runstop_r({72, 48}, {0, next_row()}, {36.0, 48.0}, {d_75}, sprite_name);
   AnimationSettings player_runstop_l({72, 48}, {0, next_row()}, {36.0, 48.0}, {d_75}, sprite_name);

   AnimationSettings player_run_r({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_60), sprite_name);
   AnimationSettings player_run_l({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_60), sprite_name);

   // frames 1,2 = dash start
   // frames 3,4 = dash loop
   // frames 2,1 = dash stop
   const auto dash_r_row = next_row();
   const auto dash_l_row = next_row();
   AnimationSettings player_dash_init_r({72, 48}, {0, dash_r_row}, {36.0, 48.0}, vx(2, d_75), sprite_name);
   AnimationSettings player_dash_init_l({72, 48}, {0, dash_l_row}, {36.0, 48.0}, vx(2, d_75), sprite_name);
   AnimationSettings player_dash_r({72, 48}, {col(2), dash_r_row}, {36.0, 48.0}, vx(2, d_75), sprite_name);
   AnimationSettings player_dash_l({72, 48}, {col(2), dash_l_row}, {36.0, 48.0}, vx(2, d_75), sprite_name);

   // init:    3 frames
   // up:      2 frames
   // midair:  8 frames
   // down:    2 frames
   // landing: 4 frames
   const auto jump_r_row = next_row();
   const auto jump_l_row = next_row();
   AnimationSettings player_jump_init_r({72, 48}, {0, jump_r_row}, {36.0, 48.0}, v(3), sprite_name);
   AnimationSettings player_jump_up_r({72, 48}, {col(3), jump_r_row}, {36.0, 48.0}, v(2), sprite_name);
   AnimationSettings player_jump_midair_r({72, 48}, {col(5), jump_r_row}, {36.0, 48.0}, v(8), sprite_name);
   AnimationSettings player_jump_down_r({72, 48}, {col(13), jump_r_row}, {36.0, 48.0},  v(2), sprite_name);
   AnimationSettings player_jump_landing_r({72, 48}, {col(15), jump_r_row}, {36.0, 48.0}, v(4), sprite_name);

   AnimationSettings player_jump_init_l({72, 48}, {0, jump_l_row}, {36.0, 48.0}, v(3), sprite_name);
   AnimationSettings player_jump_up_l({72, 48}, {col(3), jump_l_row}, {36.0, 48.0}, v(2), sprite_name);
   AnimationSettings player_jump_midair_l({72, 48}, {col(5), jump_l_row}, {36.0, 48.0}, v(8), sprite_name);
   AnimationSettings player_jump_down_l({72, 48}, {col(13), jump_l_row}, {36.0, 48.0},  v(2), sprite_name);
   AnimationSettings player_jump_landing_l({72, 48}, {col(15), jump_l_row}, {36.0, 48.0}, v(4), sprite_name);

   next_row(); // reserved
   next_row(); // reserved

   AnimationSettings player_double_jump_r({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_75), sprite_name);
   AnimationSettings player_double_jump_l({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_75), sprite_name);

   AnimationSettings player_swim_idle_r({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(5, d_75), sprite_name);
   AnimationSettings player_swim_idle_l({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(5, d_75), sprite_name);

   AnimationSettings player_swim_r({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(5, d_75), sprite_name);
   AnimationSettings player_swim_l({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(5, d_75), sprite_name);

   const auto wallslide_row_r = next_row();
   const auto wallslide_row_l = next_row();

   AnimationSettings player_wallslide_impact_r({72, 48}, {0, wallslide_row_r}, {36.0, 48.0}, vx(6, d_75), sprite_name);
   AnimationSettings player_wallslide_impact_l({72, 48}, {0, wallslide_row_l}, {36.0, 48.0}, vx(6, d_75), sprite_name);

   AnimationSettings player_wallslide_r({72, 48}, {6 * 72, wallslide_row_r}, {36.0, 48.0}, vx(2, d_75), sprite_name);
   AnimationSettings player_wallslide_l({72, 48}, {6 * 72, wallslide_row_l}, {36.0, 48.0}, vx(2, d_75), sprite_name);

   AnimationSettings player_wall_jump_r({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_75), sprite_name);
   AnimationSettings player_wall_jump_l({72, 48}, {0, next_row()}, {36.0, 48.0}, vx(12, d_75), sprite_name);

   next_row(); // reserved
   next_row(); // reserved
   next_row(); // reserved
   next_row(); // reserved
   next_row(); // reserved

   next_row(); // reserved
   next_row(); // reserved
   next_row(); // reserved

   next_row(); // reserved
   next_row(); // reserved
   next_row(); // reserved

   // const auto d2 = sf::seconds(0.2f);

   AnimationSettings player_appear_r({72, 72}, {0, next_row()}, {36.0, 72.0}, vx(12, sf::seconds(0.02f)), sprite_name);
   AnimationSettings player_appear_l({72, 72}, {0, next_row() + PIXELS_PER_TILE}, {36.0, 72.0}, vx(12, sf::seconds(0.02f)), sprite_name);

   nlohmann::json j;
   j["player_idle_r_2"]                 = player_idle_r;
   j["player_idle_l_2"]                 = player_idle_l;
   j["player_idle_blink_r_2"]           = player_idle_blink_r;
   j["player_idle_blink_l_2"]           = player_idle_blink_l;
   j["player_bend_down_r_2"]            = player_bend_down_r;
   j["player_bend_down_l_2"]            = player_bend_down_l;
   j["player_bend_down_idle_r_2"]       = player_bend_down_idle_r;
   j["player_bend_down_idle_l_2"]       = player_bend_down_idle_l;
   j["player_bend_down_idle_blink_r_2"] = player_bend_down_idle_blink_r;
   j["player_bend_down_idle_blink_l_2"] = player_bend_down_idle_blink_l;
   j["player_idle_to_run_r_2"]          = player_idle_to_run_r;
   j["player_idle_to_run_l_2"]          = player_idle_to_run_l;
   j["player_runstop_r_2"]              = player_runstop_r;
   j["player_runstop_l_2"]              = player_runstop_l;
   j["player_run_r_2"]                  = player_run_r;
   j["player_run_l_2"]                  = player_run_l;
   j["player_dash_init_r_2"]            = player_dash_init_r;
   j["player_dash_init_l_2"]            = player_dash_init_l;
   j["player_dash_r_2"]                 = player_dash_r;
   j["player_dash_l_2"]                 = player_dash_l;

   j["player_jump_init_r_2"]            = player_jump_init_r;
   j["player_jump_up_r_2"]              = player_jump_up_r;
   j["player_jump_midair_r_2"]          = player_jump_midair_r;
   j["player_jump_down_r_2"]            = player_jump_down_r;
   j["player_jump_landing_r_2"]         = player_jump_landing_r;

   j["player_jump_init_l_2"]            = player_jump_init_l;
   j["player_jump_up_l_2"]              = player_jump_up_l;
   j["player_jump_midair_l_2"]          = player_jump_midair_l;
   j["player_jump_down_l_2"]            = player_jump_down_l;
   j["player_jump_landing_l_2"]         = player_jump_landing_l;

   j["player_double_jump_r_2"]          = player_double_jump_r;
   j["player_double_jump_l_2"]          = player_double_jump_l;
   j["player_swim_idle_r_2"]            = player_swim_idle_r;
   j["player_swim_idle_l_2"]            = player_swim_idle_l;
   j["player_swim_r_2"]                 = player_swim_r;
   j["player_swim_l_2"]                 = player_swim_l;

   j["player_wallslide_r_2"]            = player_wallslide_r;
   j["player_wallslide_l_2"]            = player_wallslide_l;
   j["player_wallslide_impact_r_2"]     = player_wallslide_impact_r;
   j["player_wallslide_impact_l_2"]     = player_wallslide_impact_l;
   j["player_wall_jump_r_2"]            = player_wall_jump_r;
   j["player_wall_jump_l_2"]            = player_wall_jump_l;
   j["player_appear_r_2"]               = player_appear_r;
   j["player_appear_l_2"]               = player_appear_l;

   std::stringstream sstream;
   sstream << std::setw(4) << j << "\n\n";
   const auto data = sstream.str();

   constexpr auto json_filename = "player_unarmed.json";
   std::ofstream file(json_filename);
   file << data;

   Log::Info() << "written updated animations to:  " << json_filename;
}


void PlayerAnimation::toggleVersion()
{
   _version = (_version == Version::V1) ? Version::V2 : Version::V1;
}


void PlayerAnimation::updateV1(
   const sf::Time& dt,
   const PlayerAnimationData& data
)
{
   if (data._dead)
   {
      return;
   }

   if (Portal::isLocked())
   {
      return;
   }

   std::shared_ptr<Animation> next_cycle = nullptr;

   auto velocity = data._linear_velocity;

   const auto look_active = CameraPane::getInstance().isLookActive();
   const auto passes_sanity_check = !(data._moving_right && data._moving_left);

   auto requires_update = true;

   // dash
   if (data._dash_dir.has_value())
   {
      if (data._dash_dir == Dash::Left)
      {
         next_cycle = _dash_l;
      }
      else
      {
         next_cycle = _dash_r;
      }
   }

   // run / crouch
   else if (data._moving_right && passes_sanity_check && !data._in_air && !data._in_water && !look_active)
   {
      if (data._bending_down)
      {
         next_cycle = _crouch_r;
      }
      else
      {
         next_cycle = _run_r;
      }
   }
   else if (data._moving_left && passes_sanity_check && !data._in_air && !data._in_water && !look_active)
   {
      if (data._bending_down)
      {
         next_cycle = _crouch_l;
      }
      else
      {
         next_cycle = _run_l;
      }
   }

   // idle or idle crouch
   else if (data._points_left)
   {
      if (data._bending_down)
      {
         next_cycle = _crouch_l;
         requires_update = false;
      }
      else
      {
         next_cycle = _idle_l;
      }
   }
   else
   {
      if (data._bending_down)
      {
         next_cycle = _crouch_r;
         requires_update = false;
      }
      else
      {
         next_cycle = _idle_r;
      }
   }

   // jump init
   if (!data._dash_dir.has_value())
   {
      if (data._jump_frame_count == PhysicsConfiguration::getInstance()._player_jump_steps)
      {
         // jump ignition
         _jump_animation_reference = 0;
         next_cycle = data._points_right ? _jump_init_r : _jump_init_l;
      }
      else if ((data._in_air || data._jumping_through_one_way_wall) && !data._in_water)
      {
         // jump movement goes up
         if (velocity.y < -1.0f)
         {
            next_cycle = data._points_right ? _jump_up_r : _jump_up_l;
            _jump_animation_reference = 1;
         }
         // jump movement goes down
         else if (velocity.y > 1.0f)
         {
            next_cycle = data._points_right ? _jump_down_r : _jump_down_l;
            _jump_animation_reference = 2;
         }
         else
         {
            // jump midair
            if (_jump_animation_reference == 1)
            {
               next_cycle = data._points_right ? _jump_midair_r : _jump_midair_l;
            }
         }
      }

      // hard landing
      else if (_jump_animation_reference == 2 && data._hard_landing)
      {
         next_cycle = data._points_right ? _jump_landing_r : _jump_landing_l;

         if (next_cycle->_current_frame == static_cast<int32_t>(next_cycle->_frames.size()) - 1)
         {
             _jump_animation_reference = 3;
             next_cycle->seekToStart();
         }
      }
   }

   // swimming - no animation provided yet.
   if (data._in_water)
   {
      next_cycle = data._points_right ? _swim_r : _swim_l;
   }

   if (data._climb_joint_present)
   {
      // need to support climb animation
   }

   // force idle for screen transitions
   if (DisplayMode::getInstance().isSet(Display::ScreenTransition))
   {
      next_cycle = data._points_left ? _idle_l : _idle_r;
   }

   // reset x if animation cycle changed
   if (next_cycle != _current_cycle)
   {
      next_cycle->seekToStart();
   }

   _current_cycle = next_cycle;

   if (requires_update)
   {
      _current_cycle->update(dt);
   }
}


void PlayerAnimation::updateV2(
   const sf::Time& dt,
   const PlayerAnimationData& data
)
{
   using namespace std::chrono_literals;

   if (data._dead)
   {
      return;
   }

   if (Portal::isLocked())
   {
      return;
   }

   const auto now = StopWatch::now();

   std::shared_ptr<Animation> next_cycle = nullptr;

   auto velocity = data._linear_velocity;

   const auto look_active = CameraPane::getInstance().isLookActive();
   const auto passes_sanity_check = !(data._moving_right && data._moving_left);

   // dash
   if (data._dash_dir.has_value())
   {
      // init  regular            stop
      // |     |                  |
      // +-----+------------------+----->
      // t
      const auto dash_count_max = PhysicsConfiguration::getInstance()._player_dash_frame_count;
      const auto dash_count_regular = dash_count_max - (dash_count_max / 5);
      const auto dash_count_stop = dash_count_max / 5;

      if (data._dash_frame_count > dash_count_regular)
      {
         next_cycle = (data._dash_dir == Dash::Left) ? _dash_init_l_2 : _dash_init_r_2;
      }
      else if (data._dash_frame_count < dash_count_stop)
      {
         next_cycle = (data._dash_dir == Dash::Left) ? _dash_stop_l_2 : _dash_stop_r_2;
      }
      else
      {
         next_cycle = (data._dash_dir == Dash::Left) ? _dash_l_2 : _dash_r_2;
      }
   }

   // run
   else if (data._moving_right && passes_sanity_check && !data._in_air && !data._in_water && !look_active && !data._crouching && !data._bending_down)
   {
      next_cycle = _run_r_2;
   }
   else if (data._moving_left && passes_sanity_check && !data._in_air && !data._in_water && !look_active && !data._crouching && !data._bending_down)
   {
      next_cycle = _run_l_2;
   }

   // crouch
   else if (data._moving_right && passes_sanity_check && !data._in_air && !data._in_water && !look_active && data._crouching)
   {
      next_cycle = _crouch_r;
   }
   else if (data._moving_left && passes_sanity_check && !data._in_air && !data._in_water && !look_active && data._crouching)
   {
      next_cycle = _crouch_l;
   }

   // bend down state
   else if (data._bending_down)
   {
      next_cycle = data._points_left ? _bend_down_l_2 : _bend_down_r_2;

      if (StopWatch::duration(data._timepoint_bend_down_start, now) > 7 * 40ms)
      {
         next_cycle = data._points_left ? _bend_down_idle_l_tmp : _bend_down_idle_r_tmp;

         // blink every now and then
         if (_bend_down_idle_l_tmp->_finished)
         {
            _bend_down_idle_l_tmp = (std::rand() % 100 == 0) ? _bend_down_idle_blink_l_2 : _bend_down_idle_l_2;
         }

         if (_bend_down_idle_r_tmp->_finished)
         {
            _bend_down_idle_r_tmp = (std::rand() % 100 == 0) ? _bend_down_idle_blink_r_2 : _bend_down_idle_r_2;
         }
      }
   }

   // idle or bend back up
   else if (data._points_left)
   {
      // bend up if player is releasing the crouch
      if (StopWatch::duration(data._timepoint_bend_down_end, now) < 7 * 40ms)
      {
         next_cycle = _bend_up_l_2;
      }
      else
      {
         // otherwise randomly blink or idle
         next_cycle = _idle_l_tmp;

         if (_idle_l_tmp->_finished)
         {
            _idle_l_tmp = (std::rand() % 10 == 0) ? _idle_blink_l_2 : _idle_l_2;
         }
      }
   }
   else
   {
      // bend up if player is releasing the crouch
      if (StopWatch::duration(data._timepoint_bend_down_end, now) < 7 * 40ms)
      {
         next_cycle = _bend_up_r_2;
      }
      else
      {
         next_cycle = _idle_r_tmp;

         if (_idle_r_tmp->_finished)
         {
            _idle_r_tmp = (std::rand() % 10 == 0) ? _idle_blink_r_2 : _idle_r_2;
         }
      }
   }

   // jump init
   if (!data._dash_dir.has_value())
   {
      if (data._jump_frame_count > PhysicsConfiguration::getInstance()._player_jump_steps - FRAMES_COUNT_JUMP_INIT)
      {
         // jump ignition
         _jump_animation_reference = 0;
         next_cycle = data._points_right ? _jump_init_r_2 : _jump_init_l_2;
      }
      // jump is active when either
      // - in the air
      // - jumping through a one-sided wall (in that case player may have ground contacts)
      else if ((data._in_air || data._jumping_through_one_way_wall) && !data._in_water)
      {
         // jump movement goes up
         if (velocity.y < JUMP_UP_VELOCITY_THRESHOLD)
         {
            next_cycle = data._points_right ? _jump_up_r_2 : _jump_up_l_2;
            _jump_animation_reference = 1;
         }
         // jump movement goes down
         else if (velocity.y > JUMP_DOWN_VELOCITY_THRESHOLD)
         {
            next_cycle = data._points_right ? _jump_down_r_2 : _jump_down_l_2;
            _jump_animation_reference = 2;
         }
         else
         {
            // jump midair
            if (_jump_animation_reference == 1)
            {
               next_cycle = data._points_right ? _jump_midair_r_2 : _jump_midair_l_2;
            }
         }
      }

      // hard landing
      else if (_jump_animation_reference == 2 && data._hard_landing)
      {
         next_cycle = data._points_right ? _jump_landing_r_2 : _jump_landing_l_2;

         if (next_cycle->_current_frame == static_cast<int32_t>(next_cycle->_frames.size()) - 1)
         {
             _jump_animation_reference = 3;
             next_cycle->seekToStart();
         }
      }
   }

   // swimming - no animation provided yet.
   if (data._in_water)
   {
      next_cycle = data._points_right ? _swim_r_2 : _swim_l_2;
   }

   if (data._climb_joint_present)
   {
      // need to support climb animation
   }

   if (data._wall_sliding)
   {
      if (StopWatch::duration(data._timepoint_wallslide, now) < 6 * 75ms)
      {
         next_cycle = data._points_right ? _wallslide_impact_l_2 : _wallslide_impact_r_2;
      }
      else
      {
         next_cycle = data._points_right ? _wallslide_l_2 : _wallslide_r_2;
      }
   }

   if (StopWatch::duration(data._timepoint_doublejump, now) < 12 * 75ms)
   {
      next_cycle = data._points_right ? _double_jump_r_2 : _double_jump_l_2;
   }

   if (StopWatch::duration(data._timepoint_walljump, now) < 12 * 75ms)
   {
      next_cycle = data._wall_jump_points_right ? _wall_jump_r_2 : _wall_jump_l_2;
   }

   // appear animation
   if (GameClock::getInstance().duration() < 1.260s)
   {
      next_cycle = data._points_right ? _appear_r_2 : _appear_l_2;

      if (GameClock::getInstance().duration() < 1.0s)
      {
         // invisibility: 0 .. 1.0s (wait until player is focused)
         _appear_r_2->seekToStart();
         _appear_l_2->seekToStart();
         _appear_r_2->setAlpha(0);
         _appear_l_2->setAlpha(0);
      }
      else
      {
         // player appear animation for 12 x 20ms
         _appear_r_2->play();
         _appear_l_2->play();
         _appear_r_2->setAlpha(255);
         _appear_l_2->setAlpha(255);
      }
   }

   // force idle for screen transitions
   if (DisplayMode::getInstance().isSet(Display::ScreenTransition))
   {
      next_cycle = data._points_left ? _idle_l_tmp : _idle_r_tmp;
   }

   // reset x if animation cycle changed
   if (next_cycle != _current_cycle)
   {
      // std::cout << next_cycle->_name << std::endl;

      next_cycle->seekToStart();
      next_cycle->play();
   }

   _current_cycle = next_cycle;
   _current_cycle->update(dt);
}

