#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "animation.h"
#include "animationframedata.h"

class DetonationAnimation
{
public:
   enum class DetonationType
   {
      Big = 0,
      Small = 1
   };

   struct DetonationRing
   {
      int32_t _detonation_count = 1;           //!< detonations inside ring
      sf::Vector2f _center;                    //!< ring center
      float _radius = 0.0f;                    //!< ring radius
      float _variance_animation_speed = 0.0f;  //!< error in animation speed
      sf::Vector2f _variance_position;         //!< error in positioning
   };

   DetonationAnimation() = delete;
   DetonationAnimation(const std::vector<DetonationRing>& rings);

   const std::vector<std::shared_ptr<Animation>>& getAnimations() const;

   static DetonationAnimation makeHugeExplosion(const sf::Vector2f& center);
   static AnimationFrameData& getFrameData(DetonationAnimation::DetonationType type);

private:
   std::vector<std::shared_ptr<Animation>> _animations;
};
