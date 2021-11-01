#pragma once

#include "weatheroverlay.h"

#include <cstdint>
#include <vector>

#include <SFML/Graphics.hpp>


class RainOverlay : public WeatherOverlay
{

public:

   struct RainDrop
   {
      void reset(const sf::FloatRect& rect);

      sf::Vector2f _pos;
      sf::Vector2f _dir;
      float _length = 0.0f;
   };


   RainOverlay();

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/);
   void update(const sf::Time& dt);

private:

   std::vector<RainDrop> _drops;
   sf::FloatRect _screen;
};

