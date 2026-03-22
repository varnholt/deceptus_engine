#include "itemfactory.h"

#include "framework/tools/stringutils.h"
#include "game/items/itemlantern.h"

#include <unordered_map>

std::shared_ptr<Item> ItemFactory::create(const std::string& item_name)
{
   static const std::unordered_map<std::string, std::shared_ptr<Item>> factory_map = {
      {"lantern", std::make_shared<ItemLantern>()},
   };

   if (auto it = factory_map.find(StringUtils::toLower(item_name)); it != factory_map.end())
   {
      return it->second;
   }

   return nullptr;
}
