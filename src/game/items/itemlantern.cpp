#include "itemlantern.h"

#include <cmath>

ItemLantern::ItemLantern()
{
   _light_radius = 50.0f;
   _light_circle.setRadius(_light_radius);
   _light_circle.setFillColor(sf::Color(255, 200, 100, 150));
   _light_circle.setOrigin({_light_radius, _light_radius});
   _flicker_phase = 0.0f;
}

void ItemLantern::draw(sf::RenderTarget& target)
{
   if (!_enabled)
   {
      return;
   }

   target.draw(_light_circle);
}

void ItemLantern::update(const ItemUpdateData& data)
{
   if (!_enabled)
   {
      return;
   }

   // Follow player position
   _light_circle.setPosition(data._player_position_px);

   // Flicker effect
   _flicker_phase += data._time.asSeconds() * 10.0f;
   const auto flicker = std::sin(_flicker_phase) * 5.0f;
   const auto current_radius = _light_radius + flicker;
   _light_circle.setRadius(current_radius);
   _light_circle.setOrigin({current_radius, current_radius});
}

void ItemLantern::onEquipped()
{
   _enabled = true;
}

void ItemLantern::onUnequipped()
{
   _enabled = false;
}

std::string ItemLantern::getName() const
{
   return "Lantern";
}
