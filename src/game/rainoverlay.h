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
      RainDrop();

      sf::Vector2f mPos;
      sf::Vector2f mDir;
      float mLength = 0.0f;
   };


   RainOverlay();

   void draw(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);
   void update(const sf::Time& dt);

private:

   sf::RenderTexture mRenderTexture;
   std::vector<RainDrop> mDrops;
};

