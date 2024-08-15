#include "bouncerwrapper.h"

#include "game/level/level.h"
#include "game/mechanisms/bouncer.h"

std::shared_ptr<Bouncer> BouncerWrapper::getNearbyBouncer()
{
   auto* level = Level::getCurrentLevel();
   auto bouncers = level->getBouncers();

   std::shared_ptr<Bouncer> nearby_bouncer;

   for (auto& tmp : bouncers)
   {
      auto bouncer = std::dynamic_pointer_cast<Bouncer>(tmp);
      if (bouncer->isPlayerAtBouncer())
      {
         nearby_bouncer = bouncer;
         break;
      }
   }

   return nearby_bouncer;
}

namespace
{
using HighResDuration = std::chrono::high_resolution_clock::duration;
using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

HighResTimePoint last_use_timepoint;
HighResDuration threshold_duration = std::chrono::milliseconds(2000);
}  // namespace

void BouncerWrapper::bumpLastBouncerTime()
{
   last_use_timepoint = std::chrono::high_resolution_clock::now();
}

bool BouncerWrapper::isSpeedCapped()
{
   const auto now = std::chrono::high_resolution_clock::now();
   const auto ignored = (now - last_use_timepoint > threshold_duration);
   return ignored;
}
