#pragma once

#include <cstdint>
#include <vector>

#include <SFML/Graphics.hpp>

class RainOverlay
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
   void update();


   std::vector<RainDrop> mDrops;
};

