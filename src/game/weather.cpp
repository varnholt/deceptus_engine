#include "weather.h"

Weather::Weather()
{
   mRainOverlay = std::make_unique<RainOverlay>();
}


void Weather::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   // todo: check if player intersects with weather rect

   mRainOverlay->draw(window, states);
}


void Weather::update(const sf::Time& dt)
{
   // todo: check if player intersects with weather rect

   mRainOverlay->update(dt);
}
