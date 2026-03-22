#include "itemlantern.h"

#include <cmath>

#include "game/player/player.h"

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

void ItemLantern::update(const sf::Time& dt)
{
   if (!_enabled)
   {
      return;
   }

   // Get player position on our own
   auto* player = Player::getCurrent();
   if (player)
   {
      _light_circle.setPosition(player->getPixelPositionFloat());
   }

   // Flicker effect
   _flicker_phase += dt.asSeconds() * 10.0f;
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
