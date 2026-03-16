#include "itemfactory.h"

#include "game/items/itemlantern.h"

#include <unordered_map>

std::shared_ptr<Item> ItemFactory::create(const std::string& item_name)
{
   static const std::unordered_map<std::string, std::shared_ptr<Item>> factory_map = {
      {"Lantern", std::make_shared<ItemLantern>()},
   };

   if (auto it = factory_map.find(item_name); it != factory_map.end())
   {
      return it->second;
   }

   return nullptr;
}
