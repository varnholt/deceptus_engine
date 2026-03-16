#pragma once

#include <memory>
#include <vector>
#include <array>

#include "game/items/item.h"

class ItemSystem
{
public:
   ItemSystem() = default;

   void update(const sf::Time& dt, const sf::Vector2f& player_position_px);
   void draw(sf::RenderTarget& target);

   void syncWithInventory(const std::array<std::string, 2>& slots);

   std::shared_ptr<Item> getItem(size_t slot_index) const;

private:
   void setItem(size_t slot_index, std::shared_ptr<Item> item);

   std::array<std::shared_ptr<Item>, 2> _slots;
   std::vector<std::shared_ptr<Item>> _items;
};
