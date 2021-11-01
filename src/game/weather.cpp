#include "weather.h"

#include "framework/tmxparser/tmxobject.h"
#include "player/player.h"


Weather::Weather(GameNode* parent)
 : GameNode(parent)
{
}


void Weather::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   auto player_rect = Player::getCurrent()->getPlayerPixelRect();

   if (_rect.intersects(player_rect))
   {
      _overlay->draw(target, normal);
   }
}


void Weather::update(const sf::Time& dt)
{
   auto player_rect = Player::getCurrent()->getPlayerPixelRect();

   if (_rect.intersects(player_rect))
   {
      _overlay->update(dt);
   }
}


std::shared_ptr<Weather> Weather::deserialize(TmxObject* tmx_object)
{
   auto weather = std::make_shared<Weather>();

   weather->_rect = sf::IntRect {
      static_cast<int32_t>(tmx_object->_x_px),
      static_cast<int32_t>(tmx_object->_y_px),
      static_cast<int32_t>(tmx_object->_width_px),
      static_cast<int32_t>(tmx_object->_height_px)
   };

   weather->setZ(static_cast<int32_t>(ZDepth::ForegroundMax));

   if (tmx_object->_name.rfind("rain", 0) == 0)
   {
      weather->_overlay = std::make_shared<RainOverlay>();
   }

   return weather;
}

