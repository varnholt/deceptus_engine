#include "inventory.h"

using json = nlohmann::json;


void Inventory::add(ItemType itemType)
{
   InventoryItem item;
   item._type = itemType;

   _items.push_back(item);
}


void Inventory::clear()
{
   _items.clear();
}


const std::vector<InventoryItem>& Inventory::getItems() const
{
   return _items;
}


void Inventory::resetKeys()
{
   _items.erase(
      std::remove_if(
         _items.begin(),
         _items.end(),
         [](const auto& item) -> bool
         {
            if (
                  item._type == ItemType::KeyBlue
               || item._type == ItemType::KeyGreen
               || item._type == ItemType::KeyRed
               || item._type == ItemType::KeyYellow
               || item._type == ItemType::KeyOrange
            )
            {
               return true;
            }

            return false;
         }
      ),
      _items.end()
   );
}


void Inventory::giveAllKeys()
{
   add(ItemType::KeyRed);
   add(ItemType::KeyYellow);
   add(ItemType::KeyBlue);
   add(ItemType::KeyGreen);
   add(ItemType::KeyOrange);
}


bool Inventory::hasInventoryItem(ItemType item_type) const
{
   if (item_type == ItemType::Invalid)
   {
      return true;
   }

   const auto& it = std::find_if(std::begin(_items), std::end(_items), [item_type](auto item) {
         return (item._type == item_type);
      }
   );

   return it != _items.end();
}



void to_json(nlohmann::json& j, const Inventory& d)
{
   j = json{
      {"items", d._items}
   };
}


void from_json(const nlohmann::json& j, Inventory& d)
{
   if (j.find("items") != j.end())
   {
      d._items = j.at("items").get<std::vector<InventoryItem>>();
   }
}



