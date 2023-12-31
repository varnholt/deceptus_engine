#include "inventory.h"

#include <ranges>

using json = nlohmann::json;

void Inventory::add(const std::string& item)
{
   _items.push_back(item);
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });

   // fill empty slots with new items
   autoPopulate(item);
}

void Inventory::remove(const std::string& item)
{
   auto it = std::remove(_items.begin(), _items.end(), item);
   _items.erase(it, _items.end());

   // remove item from slots
   std::ranges::for_each(
      _slots,
      [item](auto& slot)
      {
         if (slot == item)
         {
            slot.clear();
         }
      }
   );

   // notify callbacks
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::clear()
{
   _items.clear();
   std::ranges::for_each(_slots, [](auto& slot) { slot.clear(); });

   // notify callbacks
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

const std::vector<std::string>& Inventory::getItems() const
{
   return _items;
}

void Inventory::resetKeys()
{
   _items.erase(std::remove_if(_items.begin(), _items.end(), [](const auto& key) -> bool { return key.starts_with("key"); }), _items.end());
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::selectItem(int32_t slot, const std::string& item)
{
   _slots[slot] = item;
}

void Inventory::autoPopulate(const std::string& item)
{
   // add new items to free slots
   const auto& it = std::find_if(_slots.begin(), _slots.end(), [](const auto& slot) { return slot.empty(); });
   if (it != _slots.end())
   {
      *it = item;
   }
}

bool Inventory::hasInventoryItem(const std::string& item_key) const
{
   const auto it = std::find(_items.cbegin(), _items.cend(), item_key);
   return it != _items.end();
}

void to_json(nlohmann::json& j, const Inventory& d)
{
   j = json{{"items", d._items}, {"slots", d._slots}};
}

void from_json(const nlohmann::json& j, Inventory& d)
{
   if (j.find("items") != j.end())
   {
      d._items = j.at("items").get<std::vector<std::string>>();
   }

   if (j.find("slots") != j.end())
   {
      d._slots = j.at("slots").get<std::array<std::string, 2>>();
   }
}
