#include "weather.h"

#include "player/player.h"



Weather::Weather()
{
   _rain_overlay = std::make_shared<RainOverlay>();
}


void Weather::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();

   for (const auto& data : _data)
   {
      if (data._rect.intersects(playerRect))
      {
         data._overlay->draw(window, states);
      }
   }
}


void Weather::update(const sf::Time& dt)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();

   for (const auto& data : _data)
   {
      if (data._rect.intersects(playerRect))
      {
         data._overlay->update(dt);
      }
   }
}


void Weather::add(Weather::WeatherType weatherType, const sf::IntRect& range)
{
   std::shared_ptr<WeatherOverlay> overlay;

   switch (weatherType)
   {
      case WeatherType::Rain:
         overlay = _rain_overlay;
         break;
      case WeatherType::Invalid:
         break;
   }

   _data.push_back({overlay, range});
}


void Weather::clear()
{
   _data.clear();
}


Weather& Weather::getInstance()
{
   static Weather __instance;
   return __instance;
}
