#include "itemsystem.h"

#include "game/items/itemfactory.h"

void ItemSystem::update(const sf::Time& dt, const sf::Vector2f& player_position_px)
{
   Item::ItemUpdateData item_data{dt, player_position_px};

   for (auto& item : _slots)
   {
      if (item)
      {
         item->update(item_data);
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
      else if (!_slots[i] || _slots[i]->getName() != item_name)
      {
         if (_slots[i])
         {
            _slots[i]->onUnequipped();
         }
         _slots[i] = ItemFactory::create(item_name);
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
