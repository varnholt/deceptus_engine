#include "rainoverlay.h"

#include "game/gameconfiguration.h"

#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{
   // todo: resolve static initialization fiasco
   // https://isocpp.org/wiki/faq/ctors#static-init-order
   static const auto w = 640; // GameConfiguration::getInstance().mViewWidth;
   static const auto h = 360; // GameConfiguration::getInstance().mViewHeight;

   static const auto color = sf::Color{174, 194, 224, 30};
   static const auto dropCount = 500;
   static const auto velocityFactor = 30.0f;
   static const auto widthStretchFactor = 1.5f;
   static const auto startOffsetX = -100.0f;
   static const auto startOffsetY = -20.0f;
   static const auto randomizeFactorX = 0.0f;
   static const auto randomizeFactorY = 0.02f;
   static const auto randomizeFactorLength = 0.04f;
   static const auto fixedDirectionX = 4.0f;
   static const auto fixedDirectionY = 10.0f;
   static const auto fixedLength = 0.0f;
}


RainOverlay::RainOverlay()
{
   std::srand(static_cast<uint32_t>(std::time(nullptr))); // use current time as seed for random generator

   for (auto a = 0; a < dropCount; a++)
   {
      mDrops.push_back(RainDrop());
   }

   mRenderTexture.create(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
}


void RainOverlay::draw(sf::RenderTarget& window, sf::RenderStates /*states*/)
{
   mRenderTexture.clear();

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   sf::Vertex line[2];

   for (auto& d : mDrops)
   {
      line[0] = sf::Vertex{sf::Vector2f{d.mPos.x, d.mPos.y}, color};
      line[1] = sf::Vertex{sf::Vector2f{d.mPos.x + d.mLength * d.mDir.x, d.mPos.y + d.mLength * d.mDir.y}, color};

      mRenderTexture.draw(line, 2, sf::Lines);
   }

   mRenderTexture.setView(view);
   mRenderTexture.display();

   auto sprite = sf::Sprite(mRenderTexture.getTexture());
   window.draw(sprite, sf::BlendMode{sf::BlendAdd});
}


void RainOverlay::update(const sf::Time& dt)
{
   for (auto& p : mDrops)
   {
      p.mPos += p.mDir * dt.asSeconds() * velocityFactor;

      if (p.mPos.x > w || p.mPos.y > h)
      {
         p.mPos.x = static_cast<float>(std::rand() % w) * widthStretchFactor + startOffsetX;
         p.mPos.y = startOffsetY;
      }
   }
}


RainOverlay::RainDrop::RainDrop()
{
   mPos.x = static_cast<float>(std::rand() % w);
   mPos.y = static_cast<float>(std::rand() % h);

   mLength = (std::rand() % 100) * randomizeFactorLength + fixedLength;

   auto randX = (std::rand() % 100) * randomizeFactorX;
   auto randY = (std::rand() % 100) * randomizeFactorY;

   mDir.x = randX + fixedDirectionX;
   mDir.y = randY + fixedDirectionY;
}


