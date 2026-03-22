#pragma once

#include <memory>
#include <vector>
#include <array>
#include <string>

#include "game/items/item.h"

class ItemSystem
{
public:
   ItemSystem() = default;

   void update(const sf::Time& dt);
   void draw(sf::RenderTarget& target);
   void onInventoryItemAdded(const std::string& item_name);
   void onInventoryItemRemoved(const std::string& item_name);

   void syncWithInventory(const std::array<std::string, 2>& slots);

   std::shared_ptr<Item> getItem(size_t slot_index) const;

private:
   void setItem(size_t slot_index, std::shared_ptr<Item> item);

   std::array<std::shared_ptr<Item>, 2> _slots;
   std::vector<std::shared_ptr<Item>> _items;
};
