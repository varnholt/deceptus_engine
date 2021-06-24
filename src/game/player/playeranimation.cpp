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
   const PlayerControls& controls,
   const PlayerAnimationData& data,
   const PlayerJump& jump
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
   const auto passesSanityCheck = !(controls.isMovingRight() && controls.isMovingLeft());

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
   else if (controls.isMovingRight() && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
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
   else if (controls.isMovingLeft() && passesSanityCheck && !data._in_air && !data._in_water && !lookActive)
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
      if (jump.mJumpSteps == PhysicsConfiguration::getInstance().mPlayerJumpSteps)
      {
         // jump ignition
         mJumpAnimationReference = 0;
         nextCycle = data.pointsRight() ? mJumpInitRightAligned : mJumpInitLeftAligned;
      }
      else if (data._in_air && !data._in_water)
      {
         // jump movement goes up
         if (velocity.y < -1.0f)
         {
            nextCycle = data.pointsRight() ? mJumpUpRightAligned : mJumpUpLeftAligned;
            mJumpAnimationReference = 1;
         }
         // jump movement goes down
         else if (velocity.y > 1.0f)
         {
            nextCycle = data.pointsRight() ? mJumpDownRightAligned : mJumpDownLeftAligned;
            mJumpAnimationReference = 2;
         }
         else
         {
            // jump midair
            if (mJumpAnimationReference == 1)
            {
               nextCycle = data.pointsRight() ? mJumpMidairRightAligned : mJumpMidairLeftAligned;
            }
         }
      }
      // hard landing
      else if (mJumpAnimationReference == 2 && data._hard_landing)
      {
         nextCycle = data.pointsRight() ? mJumpLandingRightAligned : mJumpLandingLeftAligned;

         if (nextCycle->_current_frame == static_cast<int32_t>(nextCycle->_frames.size()) - 1)
         {
             mJumpAnimationReference = 3;
             nextCycle->seekToStart();

             // TODO: animation class must be visualization only
             // cannot modify jump behavior here
             // data._hard_landing = false;
         }
      }
   }

   // swimming - no animation provided yet.
   if (data._in_water)
   {
      nextCycle = data.pointsRight() ? mSwimRightAligned : mSwimLeftAligned;
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
