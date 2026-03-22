#include "itemsystem.h"

#include "game/items/itemfactory.h"

#include <algorithm>

// as opposed to the weapon system, the item system is directly linked to the inventory.
// the idea is that for "supported" items, instances are being created as "Item" instance.
// Items have their own update and draw functions, so they can have their own behavior and
// visuals while being equipped.

void ItemSystem::update(const sf::Time& dt)
{
   for (auto& item : _slots)
   {
      if (item)
      {
         item->update(dt);
      }
   }
}

void ItemSystem::draw(sf::RenderTarget& target)
{
   for (auto& item : _slots)
   {
      if (item)
      {
         item->draw(target);
      }
   }
}

void ItemSystem::onInventoryItemAdded(const std::string& item_name)
{
   auto it = std::find_if(
      _items.begin(),
      _items.end(),
      [&item_name](const auto& item) { return item && item->getName() == item_name; }
   );

   if (it != _items.end())
   {
      return;
   }

   if (auto item = ItemFactory::create(item_name))
   {
      _items.push_back(item);
   }
}

void ItemSystem::onInventoryItemRemoved(const std::string& item_name)
{
   auto it = std::find_if(
      _items.begin(),
      _items.end(),
      [&item_name](const auto& item) { return item && item->getName() == item_name; }
   );

   if (it == _items.end())
   {
      return;
   }

   for (auto& slot : _slots)
   {
      if (slot == *it)
      {
         slot->onUnequipped();
         slot = nullptr;
      }
   }

   _items.erase(it);
}

void ItemSystem::syncWithInventory(const std::array<std::string, 2>& slots)
{
   for (size_t i = 0; i < slots.size(); ++i)
   {
      const auto& item_name = slots[i];

      if (item_name.empty())
      {
         if (_slots[i])
         {
            _slots[i]->onUnequipped();
         }
         _slots[i] = nullptr;
      }
      else
      {
         auto it = std::find_if(
            _items.begin(),
            _items.end(),
            [&item_name](const auto& item) { return item && item->getName() == item_name; }
         );

         auto next_item = it != _items.end() ? *it : nullptr;
         if (_slots[i] == next_item)
         {
            continue;
         }

         if (_slots[i])
         {
            _slots[i]->onUnequipped();
         }

         _slots[i] = next_item;
         if (_slots[i])
         {
            _slots[i]->onEquipped();
         }
      }
   }
}

std::shared_ptr<Item> ItemSystem::getItem(size_t slot_index) const
{
   if (slot_index >= _slots.size())
   {
      return nullptr;
   }
   return _slots[slot_index];
}

void ItemSystem::setItem(size_t slot_index, std::shared_ptr<Item> item)
{
   if (slot_index >= _slots.size())
   {
      return;
   }
   _slots[slot_index] = item;
}
