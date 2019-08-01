#pragma once

#include <SFML/Graphics.hpp>


struct BoomEffect
{
   void boom(float x, float y, float intensity = 1.0f);
   void update(const sf::Time& dt);

   sf::Time mBoomTimeEnd;
   float mBoomFactor = 1.0f;
   float mBoomOffsetX = 0.0f;
   float mBoomOffsetY = 0.0f;
   float mBoomDuration = 1.0f;
};

