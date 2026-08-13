#include "grabropewrapper.h"

#include "framework/tools/sfmlcompat.h"
#include "game/level/levelregistry.h"
#include "game/mechanisms/grabrope.h"

std::shared_ptr<GrabRope> GrabRopeWrapper::getGrabRopeAt(const sf::FloatRect& rect_px)
{
   auto level = LevelRegistry::getCurrent();

   if (!level)
   {
      return nullptr;
   }

   // all rope flavours share one group, so the cast is what picks the grabbable ones out
   for (const auto& mechanism : level->getMechanismRegistry().getRopes())
   {
      auto grab_rope = std::dynamic_pointer_cast<GrabRope>(mechanism);

      if (!grab_rope || !grab_rope->isEnabled())
      {
         continue;
      }

      if (grab_rope->isWithinGrabRange(rect_px))
      {
         return grab_rope;
      }
   }

   return nullptr;
}
