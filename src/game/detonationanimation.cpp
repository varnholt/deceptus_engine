#include "detonationanimation.h"


DetonationAnimation::DetonationAnimation(
   const std::vector<DetonationAnimation::DetonationRing>& rings,
   float boom_intensity
)
  : _boom_intensity(boom_intensity)
{
   // compute all positions
   // have one detonation per position

   for (auto& ring : rings)
   {

   }

}
