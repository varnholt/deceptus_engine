#pragma once

#include "game/animation.h"
#include "game/animationframedata.h"

#include <list>
#include <optional>


class ProjectileHitAnimation : public Animation
{
public:

   ProjectileHitAnimation() = default;

   // active animations
   static void playHitAnimation(float x, float y, float angle, const AnimationFrameData& frames);
   static void updateHitAnimations(const sf::Time& dt);
   static std::vector<ProjectileHitAnimation*>& getHitAnimations();

   // reference animations
   static void addReferenceAnimation(const std::string& id, const AnimationFrameData& animation);
   static std::map<std::string, AnimationFrameData>::const_iterator getReferenceAnimation(const std::string& id);
   static void setupDefaultAnimation();
   static AnimationFrameData getDefaultAnimation();


protected:

   static std::vector<ProjectileHitAnimation*> _active_animations;
   static std::map<std::string, AnimationFrameData> _reference_animations;

};

