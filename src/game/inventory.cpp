#include "inventory.h"

using json = nlohmann::json;


void Inventory::add(ItemType itemType)
{
   InventoryItem item;
   item.mType = itemType;

   mItems.push_back(item);
}


void Inventory::clear()
{
   mItems.clear();
}


const std::vector<InventoryItem>& Inventory::getItems() const
{
   return mItems;
}


void Inventory::resetKeys()
{
   mItems.erase(
      std::remove_if(
         mItems.begin(),
         mItems.end(),
         [](auto& item) -> bool
         {
            if (
                  item.mType == ItemType::KeyBlue
               || item.mType == ItemType::KeyGreen
               || item.mType == ItemType::KeyRed
               || item.mType == ItemType::KeyYellow
               || item.mType == ItemType::KeyOrange
            )
            {
               return true;
            }

            return false;
         }
      ),
      mItems.end()
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


bool Inventory::hasInventoryItem(ItemType itemType) const
{
   if (itemType == ItemType::Invalid)
   {
      return true;
   }

   const auto& it = std::find_if(std::begin(mItems), std::end(mItems), [itemType](auto item) {
         return (item.mType == itemType);
      }
   );

   return it != mItems.end();
}



void to_json(nlohmann::json& j, const Inventory& d)
{
   j = json{
      {"items", d.mItems}
   };
}


void from_json(const nlohmann::json& j, Inventory& d)
{
   if (j.find("items") != j.end())
   {
      d.mItems = j.at("items").get<std::vector<InventoryItem>>();
   }
}



