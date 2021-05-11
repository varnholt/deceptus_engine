#pragma once

#include "game/animation.h"
#include "game/animationframedata.h"

#include <list>


class ProjectileHitAnimation : public Animation
{
public:

   ProjectileHitAnimation() = default;

   static void add(float x, float y, float angle, const AnimationFrameData& frames);
   static void updateAnimations(const sf::Time& dt);

   static std::vector<ProjectileHitAnimation*>& getAnimations();

   static AnimationFrameData getDefaultAnimation();


protected:

   static std::vector<ProjectileHitAnimation*> _animations;
};

