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
      void resetPosition(const sf::FloatRect& rect);

      sf::Vector2f _pos_px;
      sf::Vector2f _dir_px;
      float _length = 0.0f;
      float _age_s = 0.0f;
      sf::Sprite _sprite;
      void resetDirection();
   };


   RainOverlay();

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/);
   void update(const sf::Time& dt);

private:

   void determineRainSurfaces(sf::RenderTarget& target);

   bool _initialized = false;
   sf::FloatRect _screen;
   sf::FloatRect _clip_rect;

   std::vector<RainDrop> _drops;
   std::shared_ptr<sf::Texture> _texture;
};

