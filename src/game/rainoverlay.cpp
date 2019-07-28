#include "rainoverlay.h"

#include "game/gameconfiguration.h"

#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{
   static const auto w = 320;
   static const auto h = 200;
   static const auto maxParts = 1000;
}


RainOverlay::RainOverlay()
{
   std::srand(std::time(nullptr)); // use current time as seed for random generator

   for (auto a = 0; a < maxParts; a++)
   {
      mDrops.push_back(RainDrop());
   }
}


void RainOverlay::draw(sf::RenderTarget& window, sf::RenderStates /*states*/)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   sf::Vertex line[2];

   for (auto& d : mDrops)
   {
      line[0] = sf::Vertex{sf::Vector2f{d.mPos.x, d.mPos.y}};
      line[1] = sf::Vertex{sf::Vector2f{d.mPos.x + d.mLength * d.mDir.x, d.mPos.y + d.mLength * d.mDir.y}};

      window.draw(line, 2, sf::Lines);
   }
}


void RainOverlay::update()
{
   for (auto p : mDrops)
   {
      p.mPos += p.mDir;

      if (p.mPos.x > w || p.mPos.y > h)
      {
         p.mPos.x = static_cast<float>(std::rand() % w);
         p.mPos.y = -20.0f;
      }
   }
}


RainOverlay::RainDrop::RainDrop()
{
   mPos.x = static_cast<float>(std::rand() % w);
   mPos.y = static_cast<float>(std::rand() % h);

   mLength = (std::rand() % 100) * 0.01f;

   auto randX = (std::rand() % 100) * 0.01f;
   auto randY = (std::rand() % 100) * 0.01f;

   mDir.x =  -4.0f + randX * 4.0f + 2.0f;
   mDir.y = randY * 10.0f + 10.0f;
}


