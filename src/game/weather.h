#pragma once

#include "overlays/rainoverlay.h"

#include <memory>
#include <SFML/Graphics.hpp>

class Weather
{
   public:

      enum class WeatherType {
         Rain,
         Invalid
      };

      struct WeatherData {
         std::shared_ptr<WeatherOverlay> _overlay;
         sf::IntRect _rect;
      };

      void draw(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);
      void update(const sf::Time& dt);

      void add(WeatherType weatherType, const sf::IntRect& range);
      void clear();

      static Weather& getInstance();


   private:

      Weather();

      std::shared_ptr<RainOverlay> _rain_overlay;
      std::vector<WeatherData> _data;
};

