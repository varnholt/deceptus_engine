#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

class Item
{
public:
   struct ItemUpdateData
   {
      const sf::Time& _time;
      const sf::Vector2f& _player_position_px;
   };

   Item() = default;
   virtual ~Item() = default;

   virtual void draw(sf::RenderTarget& target);
   virtual void update(const ItemUpdateData& data);
   virtual void onEquipped();
   virtual void onUnequipped();
   virtual std::string getName() const = 0;
};
