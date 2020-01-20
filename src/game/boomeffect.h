#pragma once

#include <SFML/Graphics.hpp>


struct BoomEffect
{
   void boom(float x, float y, float intensity = 1.0f);
   void update(const sf::Time& dt);
   float getRemainingTime() const;

   sf::Time mBoomTimeEnd;
   float mBoomFactor = 1.0f;
   float mBoomOffsetX = 0.0f;
   float mBoomOffsetY = 0.0f;
   float mBoomDuration = 1.0f;
   float mFactorX = 0.0f;
   float mFactorY = 0.0f;
   float mEffectVelocity = 32.0f;
   float mEffectAmplitude = 0.1f;
};

