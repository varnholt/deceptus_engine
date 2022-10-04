#include "boomeffect.h"

#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"

#include <math.h>
#include <iostream>

#include "boomeffectenveloperandom.h"
#include "boomeffectenvelopesine.h"

//-----------------------------------------------------------------------------
void BoomEffect::boom(float x, float y, float amplitude, float boom_duration, ShakeType shake_type)
{
   if (getRemainingTime() > 0.005f)
   {
      return;
   }

   switch (shake_type)
   {
      case ShakeType::Sine:
      {
         _envelope = std::make_shared<BoomEffectEnvelopeSine>();
         break;
      }
      case ShakeType::Random:
      {
         _envelope = std::make_shared<BoomEffectEnvelopeRandom>();
         break;
      }
   }

   _shake_function = [this](float x) -> float { return _envelope->shakeFunction(x); };
   _envelope->_effect_amplitude = amplitude;

   _factor_x = x;
   _factor_y = y;

   _boom_duration = boom_duration;
   _boom_time_end = GlobalClock::getInstance().getElapsedTime() + sf::seconds(_boom_duration);
}

//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::getRemainingTime() const
{
   return (_boom_time_end - GlobalClock::getInstance().getElapsedTime()).asSeconds();
}

//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::getRemainingTimeNormalized() const
{
   const auto remaining_time = getRemainingTime();
   const auto time_normalized = (_boom_duration - remaining_time) / _boom_duration;
   return time_normalized;
}

//----------------------------------------------------------------------------------------------------------------------
void BoomEffect::update(const sf::Time& /*dt*/)
{
   if (getRemainingTime() > 0.0f)
   {
      const auto& game_config = GameConfiguration::getInstance();
      const auto y = _shake_function(getRemainingTimeNormalized());

      _boom_offset_x = _factor_x * game_config._view_width * y;
      _boom_offset_y = _factor_y * game_config._view_height * y;
   }
   else
   {
      _boom_offset_x = 0;
      _boom_offset_y = 0;
   }
}
