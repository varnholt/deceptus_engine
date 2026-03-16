#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Item
{
public:
   Item() = default;
   virtual ~Item() = default;

   virtual void draw(sf::RenderTarget& target);
   virtual void update(const sf::Time& dt);
   virtual void onEquipped();
   virtual void onUnequipped();
   virtual std::string getName() const = 0;
};
