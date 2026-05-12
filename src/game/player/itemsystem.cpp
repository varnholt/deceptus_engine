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

std::shared_ptr<Item> ItemSystem::onInventoryItemAdded(const std::string& item_name)
{
   auto it = std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });

   // if item already exist, don't bother
   if (it != _items.end())
   {
      return *it;
   }

   // create item instance if supported
   auto item = ItemFactory::create(item_name);
   if (item)
   {
      _items.push_back(item);
   }
   return item;
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

void ItemSystem::reinitializeEquippedItems()
{
   for (auto& slot : _slots)
   {
      if (slot)
      {
         slot->onUnequipped();
         slot->onEquipped();
      }
   }
}

void ItemSystem::syncInventorySlots(const std::array<std::string, 2>& inventory_slots)
{
   for (auto i = 0; i < inventory_slots.size(); ++i)
   {
      const auto& item_name = inventory_slots[i];

      // resolve inventory item name to an instantiated item.
      // empty or unknown names intentionally map to nullptr (clear slot).
      std::shared_ptr<Item> instanciated_item = nullptr;

      if (!item_name.empty())
      {
         auto it =
            std::find_if(_items.begin(), _items.end(), [&item_name](const auto& item) { return item && item->getName() == item_name; });

         // create item if it doesn't exist yet (e.g., added to inventory but not equipped before)
         if (it == _items.end())
         {
            instanciated_item = onInventoryItemAdded(item_name);
         }
         else
         {
            instanciated_item = *it;
         }
      }

      // same pointer means no state transition: keep current equip state unchanged.
      if (_slots[i] == instanciated_item)
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

      _slots[i] = instanciated_item;

      if (_slots[i])
      {
         _slots[i]->onEquipped();
      }
   }
}
