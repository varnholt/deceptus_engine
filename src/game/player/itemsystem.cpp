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
   auto it = std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });

   // if item already exist, don't bother
   if (it != _items.end())
   {
      return;
   }

   // create item instance if supported
   if (auto item = ItemFactory::create(item_name))
   {
      _items.push_back(item);
   }
}

void ItemSystem::onInventoryItemRemoved(const std::string& item_name)
{
   auto it = std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });

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

void ItemSystem::syncInventorySlots(const std::array<std::string, 2>& inventory_slots)
{
   for (auto i = 0; i < inventory_slots.size(); ++i)
   {
      const auto& item_name = inventory_slots[i];

      // resolve inventory item name to an instantiated item.
      // empty or unknown names intentionally map to nullptr (clear slot).
      std::shared_ptr<Item> next_item = nullptr;

      if (!item_name.empty())
      {
         auto it =
            std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });

         // keep sync independent from inventory callback order by creating the
         // item on demand if it has not been registered yet.
         if (it == _items.end())
         {
            onInventoryItemAdded(item_name);
            it =
               std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });
         }

         next_item = it != _items.end() ? *it : nullptr;
      }

      // same pointer means no state transition: keep current equip state unchanged.
      if (_slots[i] == next_item)
      {
         continue;
      }

      // state transition order:
      // 1) unequip current item if one exists
      // 2) assign new item pointer (possibly nullptr to clear)
      // 3) equip only when the new pointer is non-null
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
