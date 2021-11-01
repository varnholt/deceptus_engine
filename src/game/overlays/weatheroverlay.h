#pragma once

#include <SFML/Graphics.hpp>

class WeatherOverlay
{
   public:

      virtual ~WeatherOverlay() = default;

      virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal) = 0;
      virtual void update(const sf::Time& dt) = 0;
};

