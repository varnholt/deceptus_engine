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
      bool _points_right = false;
      bool _climb_joint_present = false;
      bool _moving_left = false;
      bool _moving_right = false;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};
      int32_t _jump_steps = 0;
   };

   void update(
      const sf::Time& dt,
      const PlayerAnimationData& data
   );

   int32_t getJumpAnimationReference() const;

   std::shared_ptr<Animation> getCurrentCycle() const;

   void resetAlpha();

   void generateJson();


private:

   std::shared_ptr<Animation> mIdleR;
   std::shared_ptr<Animation> mIdleL;
   std::shared_ptr<Animation> mSwimR;
   std::shared_ptr<Animation> mSwimL;
   std::shared_ptr<Animation> mRunR;
   std::shared_ptr<Animation> mRunL;
   std::shared_ptr<Animation> mDashR;
   std::shared_ptr<Animation> mDashL;
   std::shared_ptr<Animation> mCrouchR;
   std::shared_ptr<Animation> mCrouchL;

   std::shared_ptr<Animation> mJumpInitR;
   std::shared_ptr<Animation> mJumpUpR;
   std::shared_ptr<Animation> mJumpMidairR;
   std::shared_ptr<Animation> mJumpDownR;
   std::shared_ptr<Animation> mJumpLandingR;

   std::shared_ptr<Animation> mJumpInitL;
   std::shared_ptr<Animation> mJumpUpL;
   std::shared_ptr<Animation> mJumpMidairL;
   std::shared_ptr<Animation> mJumpDownL;
   std::shared_ptr<Animation> mJumpLandingL;

   int32_t _jump_animation_reference = 0;

   std::vector<std::shared_ptr<Animation>> mAnimations;
   std::shared_ptr<Animation> mCurrentCycle;

};

