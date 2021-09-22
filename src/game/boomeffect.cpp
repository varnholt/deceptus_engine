#include "boomeffect.h"

#include "gameconfiguration.h"
#include "framework/tools/globalclock.h"

#include <math.h>


//-----------------------------------------------------------------------------
void BoomEffect::boom(float x, float y, float factor)
{
   if (getRemainingTime() > 0.005f)
   {
      return;
   }

   mFactorX = x;
   mFactorY = y;
   mBoomFactor = factor;
   mBoomTimeEnd = GlobalClock::getInstance()->getElapsedTime() + sf::seconds(mBoomDuration);
}


//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::getRemainingTime() const
{
   return (mBoomTimeEnd - GlobalClock::getInstance()->getElapsedTime()).asSeconds();
}


//----------------------------------------------------------------------------------------------------------------------
void BoomEffect::update(const sf::Time& /*dt*/)
{
   auto x = getRemainingTime();

   if (x > 0.0f)
   {
      GameConfiguration& gameConfig = GameConfiguration::getInstance();

      x = (mBoomDuration - x);
      x *= mEffectVelocity;
      const auto xSquare = x * x;

      const auto fx = mBoomFactor * mEffectAmplitude * 2.0f * sin(xSquare) * (1.0f / (1.0f + xSquare));

      mBoomOffsetX = mFactorX * gameConfig._view_width * fx;
      mBoomOffsetY = mFactorY * gameConfig._view_height * fx;
   }
   else
   {
      mBoomOffsetX = 0;
      mBoomOffsetY = 0;
   }
}

