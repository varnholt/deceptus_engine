#include "playeranimation.h"

#include <fstream>
#include <sstream>

#include "animationpool.h"
#include "camerapane.h"
#include "mechanisms/portal.h"
#include "physics/physicsconfiguration.h"


PlayerAnimation::PlayerAnimation()
{
   // none of the player animations are managed by the animation pool, they're just paused when finished
   mIdleR        = AnimationPool::getInstance().add("player_idle_r",         0.0f, 0.0f, true, false);
   mIdleL        = AnimationPool::getInstance().add("player_idle_l",         0.0f, 0.0f, true, false);
   mSwimR        = AnimationPool::getInstance().add("player_swim_r",         0.0f, 0.0f, true, false);
   mSwimL        = AnimationPool::getInstance().add("player_swim_l",         0.0f, 0.0f, true, false);
   mRunR         = AnimationPool::getInstance().add("player_run_r",          0.0f, 0.0f, true, false);
   mRunL         = AnimationPool::getInstance().add("player_run_l",          0.0f, 0.0f, true, false);
   mDashR        = AnimationPool::getInstance().add("player_dash_r",         0.0f, 0.0f, true, false);
   mDashL        = AnimationPool::getInstance().add("player_dash_l",         0.0f, 0.0f, true, false);
   mCrouchR      = AnimationPool::getInstance().add("player_crouch_r",       0.0f, 0.0f, true, false);
   mCrouchL      = AnimationPool::getInstance().add("player_crouch_l",       0.0f, 0.0f, true, false);

   mJumpInitR    = AnimationPool::getInstance().add("player_jump_init_r",    0.0f, 0.0f, true, false);
   mJumpUpR      = AnimationPool::getInstance().add("player_jump_up_r",      0.0f, 0.0f, true, false);
   mJumpMidairR  = AnimationPool::getInstance().add("player_jump_midair_r",  0.0f, 0.0f, true, false);
   mJumpDownR    = AnimationPool::getInstance().add("player_jump_down_r",    0.0f, 0.0f, true, false);
   mJumpLandingR = AnimationPool::getInstance().add("player_jump_landing_r", 0.0f, 0.0f, true, false);

   mJumpInitL    = AnimationPool::getInstance().add("player_jump_init_l",    0.0f, 0.0f, true, false);
   mJumpUpL      = AnimationPool::getInstance().add("player_jump_up_l",      0.0f, 0.0f, true, false);
   mJumpMidairL  = AnimationPool::getInstance().add("player_jump_midair_l",  0.0f, 0.0f, true, false);
   mJumpDownL    = AnimationPool::getInstance().add("player_jump_down_l",    0.0f, 0.0f, true, false);
   mJumpLandingL = AnimationPool::getInstance().add("player_jump_landing_l", 0.0f, 0.0f, true, false);

   mAnimations.push_back(mIdleR);
   mAnimations.push_back(mIdleL);
   mAnimations.push_back(mSwimR);
   mAnimations.push_back(mSwimL);
   mAnimations.push_back(mRunR);
   mAnimations.push_back(mRunL);
   mAnimations.push_back(mDashR);
   mAnimations.push_back(mDashL);
   mAnimations.push_back(mCrouchR);
   mAnimations.push_back(mCrouchL);

   mAnimations.push_back(mJumpInitR);
   mAnimations.push_back(mJumpUpR);
   mAnimations.push_back(mJumpDownR);
   mAnimations.push_back(mJumpLandingR);
   mAnimations.push_back(mJumpMidairR);

   mAnimations.push_back(mJumpInitL);
   mAnimations.push_back(mJumpUpL);
   mAnimations.push_back(mJumpDownL);
   mAnimations.push_back(mJumpLandingL);
   mAnimations.push_back(mJumpMidairL);

   for (auto& i : mAnimations)
   {
      i->_looped = true;
   }
}


void PlayerAnimation::update(
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

   std::shared_ptr<Animation> nextCycle = nullptr;

   auto velocity = data._linear_velocity;

   const auto lookActive = CameraPane::getInstance().isLookActive();
   const auto passesSanityCheck = !(data._moving_right && data._moving_left);

   auto requiresUpdate = true;

   // dash
   if (data._dash_dir.has_value())
   {
      if (data._dash_dir == Dash::Left)
      {
         nextCycle = mDashL;
      }
      else
      {
         nextCycle = mDashR;
      }
   }

   // run / crouch
   else if (data._moving_right && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchR;
      }
      else
      {
         nextCycle = mRunR;
      }
   }
   else if (data._moving_left && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchL;
      }
      else
      {
         nextCycle = mRunL;
      }
   }

   // idle or idle crouch
   else if (data._points_left)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchL;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleL;
      }
   }
   else
   {
      if (data._crouching)
      {
         nextCycle = mCrouchR;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleR;
      }
   }

   // jump init
   if (!data._dash_dir.has_value())
   {
      if (data._jump_steps == PhysicsConfiguration::getInstance().mPlayerJumpSteps)
      {
         // jump ignition
         _jump_animation_reference = 0;
         nextCycle = data._points_right ? mJumpInitR : mJumpInitL;
      }
      else if (data._in_air && !data._in_water)
      {
         // jump movement goes up
         if (velocity.y < -1.0f)
         {
            nextCycle = data._points_right ? mJumpUpR : mJumpUpL;
            _jump_animation_reference = 1;
         }
         // jump movement goes down
         else if (velocity.y > 1.0f)
         {
            nextCycle = data._points_right ? mJumpDownR : mJumpDownL;
            _jump_animation_reference = 2;
         }
         else
         {
            // jump midair
            if (_jump_animation_reference == 1)
            {
               nextCycle = data._points_right ? mJumpMidairR : mJumpMidairL;
            }
         }
      }

      // hard landing
      else if (_jump_animation_reference == 2 && data._hard_landing)
      {
         nextCycle = data._points_right ? mJumpLandingR : mJumpLandingL;

         if (nextCycle->_current_frame == static_cast<int32_t>(nextCycle->_frames.size()) - 1)
         {
             _jump_animation_reference = 3;
             nextCycle->seekToStart();
         }
      }
   }

   // swimming - no animation provided yet.
   if (data._in_water)
   {
      nextCycle = data._points_right ? mSwimR : mSwimL;
   }

   if (data._climb_joint_present)
   {
      // need to support climb animation
   }

   // reset x if animation cycle changed
   if (nextCycle != mCurrentCycle)
   {
      nextCycle->seekToStart();
   }

   mCurrentCycle = nextCycle;

   if (requiresUpdate)
   {
      mCurrentCycle->update(dt);
   }
}


int32_t PlayerAnimation::getJumpAnimationReference() const
{
   return _jump_animation_reference;
}


std::shared_ptr<Animation> PlayerAnimation::getCurrentCycle() const
{
   return mCurrentCycle;
}


void PlayerAnimation::resetAlpha()
{
   // reset alphas if needed
   for (auto& a: mAnimations)
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

   std::vector<AnimationSettings> settings;

   const auto d = sf::seconds(0.075f);

   AnimationSettings player_idle_r({72, 48}, {0, 0}, {36.0, 48.0}, {d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_idle_l({72, 48}, {0, 24}, {36.0, 48.0}, {d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_bend_down_r({72, 48}, {0, 48}, {36.0, 48.0}, {d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_bend_down_l({72, 48}, {0, 72}, {36.0, 48.0}, {d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_idle_to_run_r({72, 48}, {0, 96}, {36.0, 48.0}, {d,d}, "player_unarmed.png");
   AnimationSettings player_idle_to_run_l({72, 48}, {0, 120}, {36.0, 48.0}, {d,d}, "player_unarmed.png");
   AnimationSettings player_runstop_r({72, 48}, {0, 144}, {36.0, 48.0}, {d}, "player_unarmed.png");
   AnimationSettings player_runstop_l({72, 48}, {0, 168}, {36.0, 48.0}, {d}, "player_unarmed.png");
   AnimationSettings player_run_r({72, 48}, {0, 192}, {36.0, 48.0}, {d,d,d,d,d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_run_l({72, 48}, {0, 216}, {36.0, 48.0}, {d,d,d,d,d,d,d,d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_dash_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_dash_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_jump_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_jump_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_double_jump_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_double_jump_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_swim_idle_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_swim_idle_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_swim_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_swim_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_wallslide_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_wallslide_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_wall_jump_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_wall_jump_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_appear_r({72, 48}, {0, 240}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");
   AnimationSettings player_appear_l({72, 48}, {0, 264}, {36.0, 48.0}, {d,d,d,d,d}, "player_unarmed.png");

   nlohmann::json j;
   j["player_idle_r"]         = player_idle_r;
   j["player_idle_l"]         = player_idle_l;
   j["player_bend_down_r"]    = player_bend_down_r;
   j["player_bend_down_l"]    = player_bend_down_l;
   j["player_idle_to_run_r"]  = player_idle_to_run_r;
   j["player_idle_to_run_l"]  = player_idle_to_run_l;
   j["player_runstop_r"]      = player_runstop_r;
   j["player_runstop_l"]      = player_runstop_l;
   j["player_run_r"]          = player_run_r;
   j["player_run_l"]          = player_run_l;
   j["player_dash_r"]         = player_dash_r;
   j["player_dash_l"]         = player_dash_l;
   j["player_jump_r"]         = player_jump_r;
   j["player_jump_l"]         = player_jump_l;
   j["player_double_jump_r"]  = player_double_jump_r;
   j["player_double_jump_l"]  = player_double_jump_l;
   j["player_swim_idle_r"]    = player_swim_idle_r;
   j["player_swim_idle_l"]    = player_swim_idle_l;
   j["player_swim_r"]         = player_swim_r;
   j["player_swim_l"]         = player_swim_l;
   j["player_wallslide_r"]    = player_wallslide_r;
   j["player_wallslide_l"]    = player_wallslide_l;
   j["player_wall_jump_r"]    = player_wall_jump_r;
   j["player_wall_jump_l"]    = player_wall_jump_l;
   j["player_appear_r"]       = player_appear_r;
   j["player_appear_l"]       = player_appear_l;

   std::stringstream sstream;
   sstream << std::setw(4) << j << "\n\n";
   const auto data = sstream.str();

   std::ofstream file("player_unarmed.json");
   file << data;
}

