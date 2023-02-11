#include "portalwrapper.h"

#include "level.h"

std::shared_ptr<Portal> PortalWrapper::getNearbyPortal()
{
   std::shared_ptr<Portal> nearby_portal;

   const auto& portals = Level::getCurrentLevel()->getPortals();

   for (auto& p : portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      if (portal->isPlayerAtPortal())
      {
         nearby_portal = portal;
         break;
      }
   }

   return nearby_portal;
}
