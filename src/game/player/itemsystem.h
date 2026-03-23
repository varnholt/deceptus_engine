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
   void syncInventorySlots(const std::array<std::string, 2>& slots);

private:

   std::array<std::shared_ptr<Item>, 2> _slots;
   std::vector<std::shared_ptr<Item>> _items;
};
