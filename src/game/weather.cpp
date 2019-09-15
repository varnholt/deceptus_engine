#include "weather.h"

#include "player.h"


Weather Weather::sInstance;


Weather::Weather()
{
   mRainOverlay = std::make_shared<RainOverlay>();
}


void Weather::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();

   for (const auto& data : mData)
   {
      if (data.mRect.intersects(playerRect))
      {
         data.mOverlay->draw(window, states);
      }
   }
}


void Weather::update(const sf::Time& dt)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();

   for (const auto& data : mData)
   {
      if (data.mRect.intersects(playerRect))
      {
         data.mOverlay->update(dt);
      }
   }
}


void Weather::add(Weather::WeatherType weatherType, const sf::IntRect& range)
{
   std::shared_ptr<WeatherOverlay> overlay;

   switch (weatherType)
   {
      case WeatherType::Rain:
         overlay = mRainOverlay;
         break;
      case WeatherType::Invalid:
         break;
   }

   mData.push_back({overlay, range});
}


void Weather::clear()
{
   mData.clear();
}


Weather& Weather::getInstance()
{
   return sInstance;
}
