#pragma once

#include <cstdint>
#include <SFML/Graphics.hpp>

#include "animationframedata.h"

class DetonationAnimation
{
public:

   struct DetonationRing
   {
      sf::Vector2f _center;
      float _radius = 0.0f;
      AnimationFrameData _frame_data;
      float _variance_animation_speed = 0.0f;
      sf::Vector2f _variance_position;
   };

   DetonationAnimation() = default;

   DetonationAnimation(
      const std::vector<DetonationRing>& rings,
      float boom_intensity
   );


private:

   std::vector<int32_t> _detonation_counts_small;
   std::vector<int32_t> _detonation_counts_large;
   float _boom_intensity = 1.0f;
};

