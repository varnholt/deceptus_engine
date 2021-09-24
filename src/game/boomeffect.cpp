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

   _factor_x = x;
   _factor_y = y;
   _boom_factor = factor;
   _boom_time_end = GlobalClock::getInstance()->getElapsedTime() + sf::seconds(_boom_duration);
}


//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::getRemainingTime() const
{
   return (_boom_time_end - GlobalClock::getInstance()->getElapsedTime()).asSeconds();
}


//----------------------------------------------------------------------------------------------------------------------
void BoomEffect::update(const sf::Time& /*dt*/)
{
   auto x = getRemainingTime();

   if (x > 0.0f)
   {
      GameConfiguration& game_config = GameConfiguration::getInstance();

      x = (_boom_duration - x);
      x *= _effect_velocity;
      const auto x_square = x * x;

      const auto fx = _boom_factor * _effect_amplitude * 2.0f * sin(x_square) * (1.0f / (1.0f + x_square));

      _boom_offset_x = _factor_x * game_config._view_width * fx;
      _boom_offset_y = _factor_y * game_config._view_height * fx;
   }
   else
   {
      _boom_offset_x = 0;
      _boom_offset_y = 0;
   }
}

