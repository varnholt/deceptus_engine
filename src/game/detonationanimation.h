#pragma once

#include <cstdint>
#include <SFML/Graphics.hpp>

#include "animation.h"
#include "animationframedata.h"

class DetonationAnimation
{
public:

   struct DetonationRing
   {
      int32_t _detonation_count = 1;
      sf::Vector2f _center;
      float _radius = 0.0f;
      AnimationFrameData _frame_data;
      float _variance_animation_speed = 0.0f;
      sf::Vector2f _variance_position;
   };

   DetonationAnimation() = default;

   DetonationAnimation(const std::vector<DetonationRing>& rings);

   static void unitTest1();

private:

   std::vector<Animation> _animations;
};

