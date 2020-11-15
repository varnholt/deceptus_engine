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
         std::shared_ptr<WeatherOverlay> mOverlay;
         sf::IntRect mRect;
      };

      void draw(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);
      void update(const sf::Time& dt);

      void add(WeatherType weatherType, const sf::IntRect& range);
      void clear();

      static Weather& getInstance();


   private:

      Weather();

      static Weather sInstance;
      std::shared_ptr<RainOverlay> mRainOverlay;
      std::vector<WeatherData> mData;
};

