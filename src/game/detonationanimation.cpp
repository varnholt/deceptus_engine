#include "detonationanimation.h"

#include <iostream>
#include <math.h>

DetonationAnimation::DetonationAnimation(
   const std::vector<DetonationAnimation::DetonationRing>& rings
)
{
   // compute all positions
   // have one detonation per position

   for (auto& ring : rings)
   {
      float angle = 0.0f;
      float angle_increment = static_cast<float>(M_PI) / static_cast<float>(ring._detonation_count);

      for (auto i = 0; i < ring._detonation_count; i++)
      {
         const auto x = ring._center.x + ring._variance_position.x + (cos(angle) * ring._radius);
         const auto y = ring._center.x + ring._variance_position.y + (sin(angle) * ring._radius);

         angle += angle_increment;

         std::cout << x << std::endl;
         std::cout << y << std::endl;
      }
   }
}


void DetonationAnimation::unitTest1()
{
   DetonationRing ring_a;
   DetonationRing ring_b;
   DetonationRing ring_c;

   ring_a._radius = 0;
   ring_b._radius = 2;
   ring_c._radius = 5;

   ring_a._detonation_count = 1;
   ring_b._detonation_count = 4;
   ring_c._detonation_count = 9;

   std::vector<DetonationRing> detonation_ring;

   detonation_ring.push_back(ring_a);
   detonation_ring.push_back(ring_b);
   detonation_ring.push_back(ring_c);

   DetonationAnimation animation(detonation_ring);

   (void)animation;
}


