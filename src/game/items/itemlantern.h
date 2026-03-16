#pragma once

#include "game/items/item.h"

#include <SFML/Graphics.hpp>

class ItemLantern : public Item
{
public:
   ItemLantern();

   void draw(sf::RenderTarget& target) override;
   void update(const ItemUpdateData& data) override;
   void onEquipped() override;
   void onUnequipped() override;
   std::string getName() const override;

private:
   sf::CircleShape _light_circle;
   float _light_radius;
   float _flicker_phase;
   bool _enabled{false};
};
