#include "playeranimation.h"

#include "animationpool.h"
#include "camerapane.h"
#include "mechanisms/portal.h"
#include "physics/physicsconfiguration.h"


PlayerAnimation::PlayerAnimation()
{
   // none of the player animations are managed by the animation pool, they're just paused when finished
   mIdleRightAligned        = AnimationPool::getInstance().add("player_idle_right_aligned",         0.0f, 0.0f, true, false);
   mIdleLeftAligned         = AnimationPool::getInstance().add("player_idle_left_aligned",          0.0f, 0.0f, true, false);
   mSwimRightAligned        = AnimationPool::getInstance().add("player_swim_right_aligned",         0.0f, 0.0f, true, false);
   mSwimLeftAligned         = AnimationPool::getInstance().add("player_swim_left_aligned",          0.0f, 0.0f, true, false);
   mRunRightAligned         = AnimationPool::getInstance().add("player_run_right_aligned",          0.0f, 0.0f, true, false);
   mRunLeftAligned          = AnimationPool::getInstance().add("player_run_left_aligned",           0.0f, 0.0f, true, false);
   mDashRightAligned        = AnimationPool::getInstance().add("player_dash_right_aligned",         0.0f, 0.0f, true, false);
   mDashLeftAligned         = AnimationPool::getInstance().add("player_dash_left_aligned",          0.0f, 0.0f, true, false);
   mCrouchRightAligned      = AnimationPool::getInstance().add("player_crouch_right_aligned",       0.0f, 0.0f, true, false);
   mCrouchLeftAligned       = AnimationPool::getInstance().add("player_crouch_left_aligned",        0.0f, 0.0f, true, false);

   mJumpInitRightAligned    = AnimationPool::getInstance().add("player_jump_init_right_aligned",    0.0f, 0.0f, true, false);
   mJumpUpRightAligned      = AnimationPool::getInstance().add("player_jump_up_right_aligned",      0.0f, 0.0f, true, false);
   mJumpMidairRightAligned  = AnimationPool::getInstance().add("player_jump_midair_right_aligned",  0.0f, 0.0f, true, false);
   mJumpDownRightAligned    = AnimationPool::getInstance().add("player_jump_down_right_aligned",    0.0f, 0.0f, true, false);
   mJumpLandingRightAligned = AnimationPool::getInstance().add("player_jump_landing_right_aligned", 0.0f, 0.0f, true, false);

   mJumpInitLeftAligned     = AnimationPool::getInstance().add("player_jump_init_left_aligned",     0.0f, 0.0f, true, false);
   mJumpUpLeftAligned       = AnimationPool::getInstance().add("player_jump_up_left_aligned",       0.0f, 0.0f, true, false);
   mJumpMidairLeftAligned   = AnimationPool::getInstance().add("player_jump_midair_left_aligned",   0.0f, 0.0f, true, false);
   mJumpDownLeftAligned     = AnimationPool::getInstance().add("player_jump_down_left_aligned",     0.0f, 0.0f, true, false);
   mJumpLandingLeftAligned  = AnimationPool::getInstance().add("player_jump_landing_left_aligned",  0.0f, 0.0f, true, false);

   mAnimations.push_back(mIdleRightAligned);
   mAnimations.push_back(mIdleLeftAligned);
   mAnimations.push_back(mSwimRightAligned);
   mAnimations.push_back(mSwimLeftAligned);
   mAnimations.push_back(mRunRightAligned);
   mAnimations.push_back(mRunLeftAligned);
   mAnimations.push_back(mDashRightAligned);
   mAnimations.push_back(mDashLeftAligned);
   mAnimations.push_back(mCrouchRightAligned);
   mAnimations.push_back(mCrouchLeftAligned);

   mAnimations.push_back(mJumpInitRightAligned);
   mAnimations.push_back(mJumpUpRightAligned);
   mAnimations.push_back(mJumpDownRightAligned);
   mAnimations.push_back(mJumpLandingRightAligned);
   mAnimations.push_back(mJumpMidairRightAligned);

   mAnimations.push_back(mJumpInitLeftAligned);
   mAnimations.push_back(mJumpUpLeftAligned);
   mAnimations.push_back(mJumpDownLeftAligned);
   mAnimations.push_back(mJumpLandingLeftAligned);
   mAnimations.push_back(mJumpMidairLeftAligned);

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
         nextCycle = mDashLeftAligned;
      }
      else
      {
         nextCycle = mDashRightAligned;
      }
   }

   // run / crouch
   else if (data._moving_right && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchRightAligned;
      }
      else
      {
         nextCycle = mRunRightAligned;
      }
   }
   else if (data._moving_left && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchLeftAligned;
      }
      else
      {
         nextCycle = mRunLeftAligned;
      }
   }

   // idle or idle crouch
   else if (data._points_left)
   {
      if (data._crouching)
      {
         nextCycle = mCrouchLeftAligned;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleLeftAligned;
      }
   }
   else
   {
      if (data._crouching)
      {
         nextCycle = mCrouchRightAligned;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleRightAligned;
      }
   }

   // jump init
   if (!data._dash_dir.has_value())
   {
      if (data._jump_steps == PhysicsConfiguration::getInstance().mPlayerJumpSteps)
      {
         // jump ignition
         _jump_animation_reference = 0;
         nextCycle = data._points_right ? mJumpInitRightAligned : mJumpInitLeftAligned;
      }
      else if (data._in_air && !data._in_water)
      {
         // jump movement goes up
         if (velocity.y < -1.0f)
         {
            nextCycle = data._points_right ? mJumpUpRightAligned : mJumpUpLeftAligned;
            _jump_animation_reference = 1;
         }
         // jump movement goes down
         else if (velocity.y > 1.0f)
         {
            nextCycle = data._points_right ? mJumpDownRightAligned : mJumpDownLeftAligned;
            _jump_animation_reference = 2;
         }
         else
         {
            // jump midair
            if (_jump_animation_reference == 1)
            {
               nextCycle = data._points_right ? mJumpMidairRightAligned : mJumpMidairLeftAligned;
            }
         }
      }

      // hard landing
      else if (_jump_animation_reference == 2 && data._hard_landing)
      {
         nextCycle = data._points_right ? mJumpLandingRightAligned : mJumpLandingLeftAligned;

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
      nextCycle = data._points_right ? mSwimRightAligned : mSwimLeftAligned;
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


// 00 - player_idle_right_aligned, 8
// 01 - player_idle_left_aligned, 8
// 02 - player_bend_down_right_aligned, 8
// 03 - player_bend_down_left_aligned, 8
// 04 - player_idle_to_run_right_aligned, 2
// 05 - player_idle_to_run_left_aligned, 2
// 06 - player_runstop_right_aligned, 0
// 07 - player_runstop_left_aligned, 0
// 08 - player_run_right_aligned, 12
// 09 - player_run_left_aligned, 12
// 10 - player_dash_right_aligned, 5
// 11 - player_dash_left_aligned, 5
// 12 - player_jump_right_aligned, 0
// 13 - player_jump_left_aligned, 0
// 14 - player_double_jump_right_aligned, 0
// 15 - player_double_jump_left_aligned, 0
// 16 - player_swim_idle_right_aligned, 12
// 17 - player_swim_idle_left_aligned, 12
// 18 - player_swim_right_aligned, 0
// 19 - player_swim_left_aligned, 0
// 20 - player_wallslide_right_aligned, 6
// 21 - player_wallslide_left_aligned, 6
// 22 - player_wall_jump_right_aligned, 0
// 23 - player_wall_jump_left_aligned, 0
// 24 - player_appear_right_aligned, 12
// 25 - player_appear_left_aligned, 12
//
//       "frame_size": [72, 48],
//       "frame_offset": [0, 480],
//       "sprite_count": 1,
//       "frame_durations": [100],
//       "origin": [36.0, 48.0],
//       "texture": "data/sprites/player_spriteset.png"
