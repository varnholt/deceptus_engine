#pragma once

#include "rainoverlay.h"

#include <memory>
#include <SFML/Graphics.hpp>

class Weather
{
   public:
      Weather();

      void draw(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);
      void update(const sf::Time& dt);

   private:

      std::unique_ptr<RainOverlay> mRainOverlay;
};

