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
