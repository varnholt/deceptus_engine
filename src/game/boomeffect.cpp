#include "boomeffect.h"

#include "gameconfiguration.h"
#include "globalclock.h"

#include <math.h>


//-----------------------------------------------------------------------------
void BoomEffect::boom(float /*x*/, float /*y*/, float factor)
{
   mBoomFactor = factor;
   mBoomTimeEnd = GlobalClock::getInstance()->getElapsedTime() + sf::seconds(mBoomDuration);
}


//----------------------------------------------------------------------------------------------------------------------
void BoomEffect::update(const sf::Time& /*dt*/)
{
   auto x = (mBoomTimeEnd - GlobalClock::getInstance()->getElapsedTime()).asSeconds();

   if (x > 0.0f)
   {
      static const auto effectVelocity = 32.0f;
      static const auto effectAmplitude = 0.1f;

      GameConfiguration& gameConfig = GameConfiguration::getInstance();

      x = (mBoomDuration - x);
      x *= effectVelocity;

      const auto fx = mBoomFactor * effectAmplitude * 2.0f * sin(x * x) * (1.0f / (1.0f + x * x));
      mBoomOffsetY = gameConfig.mViewHeight * fx;
   }
   else
   {
      mBoomOffsetY = 0;
   }
}
