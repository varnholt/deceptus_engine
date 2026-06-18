#include "extrawrapper.h"

#include "game/level/levelregistry.h"
#include "game/mechanisms/extra.h"
#include "game/mechanisms/portal.h"

std::shared_ptr<Extra> ExtraWrapper::spawnExtra(const std::string& id, sf::Vector2f offset)
{
   const auto extras = LevelRegistry::getCurrent()->getMechanismRegistry().getExtras();

   std::shared_ptr<Portal> nearby_portal;

   for (const auto& tmp : extras)
   {
      auto extra = std::dynamic_pointer_cast<Extra>(tmp);
      if (extra->_name == id)
      {
         extra->spawn(offset);
         return extra;
      }
   }
   return nullptr;
}
