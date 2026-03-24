#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "animation.h"
#include "animationframedata.h"

/// \brief builds clustered explosion animations arranged in configurable rings around a center point.
class DetonationAnimation
{
public:
   enum class DetonationType
   {
      Big = 0,
      Small = 1
   };

   /// \brief ring definition that controls how many detonations are spawned and how they are randomized.
   struct DetonationRing
   {
      /// detonations inside ring
      int32_t _detonation_count = 1;
      /// ring center
      sf::Vector2f _center;
      /// ring radius
      float _radius = 0.0f;
      /// error in animation speed
      float _variance_animation_speed = 0.0f;
      /// error in positioning
      sf::Vector2f _variance_position;
   };

   DetonationAnimation() = delete;
   /// \brief creates one animation per configured ring detonation with randomized position and timing offsets.
   /// \param rings concentric ring descriptions used to spawn big center and small outer explosions.
   DetonationAnimation(const std::vector<DetonationRing>& rings);

   /// \brief returns all explosion animation instances created for this detonation event.
   /// \return reference to the internal vector of spawned animations.
   const std::vector<std::shared_ptr<Animation>>& getAnimations() const;

   /// \brief creates a predefined multi-ring explosion setup used for a large blast effect.
   /// \param center world position in pixels used as the explosion center.
   /// \return detonation animation containing the predefined huge-explosion rings.
   static DetonationAnimation makeHugeExplosion(const sf::Vector2f& center);
   /// \brief returns cached frame data for a random variation of the requested explosion type.
   /// \param type selects big or small explosion sprite layout.
   /// \return reference to one cached frame-data variant.
   static AnimationFrameData& getFrameData(DetonationAnimation::DetonationType type);

private:
   std::vector<std::shared_ptr<Animation>> _animations;
};
