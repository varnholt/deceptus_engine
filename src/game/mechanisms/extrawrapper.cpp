#include "extrawrapper.h"

#include "game/level/level.h"
#include "game/mechanisms/extra.h"

void ExtraWrapper::spawnExtra(const std::string& id)
{
   auto* level = Level::getCurrentLevel();
   const auto extras = level->getExtras();

   std::shared_ptr<Portal> nearby_portal;

   for (const auto& tmp : extras)
   {
      auto extra = std::dynamic_pointer_cast<Extra>(tmp);
      if (extra->_name == id)
      {
         extra->spawn();
         break;
      }
   }
}
