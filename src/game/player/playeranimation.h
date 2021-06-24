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

   struct PlayerAnimationData
   {
      bool _dead = false;
      bool _in_air = false;
      bool _in_water = false;
      bool _hard_landing = false;
      bool _crouching = false;
      bool _points_left = false;
      bool _climb_joint_present = false;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};

      bool pointsRight() const
      {
         return !_points_left;
      }
   };

   void update(
      const sf::Time& dt,
      const PlayerControls& controls,
      const PlayerAnimationData& data,
      const PlayerJump& jump
   );

private:

   std::shared_ptr<Animation> mIdleRightAligned;
   std::shared_ptr<Animation> mIdleLeftAligned;
   std::shared_ptr<Animation> mSwimRightAligned;
   std::shared_ptr<Animation> mSwimLeftAligned;
   std::shared_ptr<Animation> mRunRightAligned;
   std::shared_ptr<Animation> mRunLeftAligned;
   std::shared_ptr<Animation> mDashRightAligned;
   std::shared_ptr<Animation> mDashLeftAligned;
   std::shared_ptr<Animation> mCrouchRightAligned;
   std::shared_ptr<Animation> mCrouchLeftAligned;

   std::shared_ptr<Animation> mJumpInitRightAligned;
   std::shared_ptr<Animation> mJumpUpRightAligned;
   std::shared_ptr<Animation> mJumpMidairRightAligned;
   std::shared_ptr<Animation> mJumpDownRightAligned;
   std::shared_ptr<Animation> mJumpLandingRightAligned;

   std::shared_ptr<Animation> mJumpInitLeftAligned;
   std::shared_ptr<Animation> mJumpUpLeftAligned;
   std::shared_ptr<Animation> mJumpMidairLeftAligned;
   std::shared_ptr<Animation> mJumpDownLeftAligned;
   std::shared_ptr<Animation> mJumpLandingLeftAligned;

   int32_t mJumpAnimationReference = 0;

   std::vector<std::shared_ptr<Animation>> mAnimations;
   std::shared_ptr<Animation> mCurrentCycle;

};

