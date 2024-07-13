#include "portalwrapper.h"

#include "game/level/level.h"
#include "game/mechanisms/portal.h"

std::shared_ptr<Portal> PortalWrapper::getNearbyPortal()
{
   auto level = Level::getCurrentLevel();
   const auto portals = level->getPortals();

   std::shared_ptr<Portal> nearby_portal;

   for (auto& tmp : portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(tmp);
      if (portal->isPlayerAtPortal())
      {
         nearby_portal = portal;
         break;
      }
   }

   return nearby_portal;
}
