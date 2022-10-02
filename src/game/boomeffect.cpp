#include "boomeffect.h"

#include "gameconfiguration.h"
#include "framework/tools/globalclock.h"

#include <math.h>
#include <iostream>

//-----------------------------------------------------------------------------
void BoomEffect::boom(float x, float y, float factor)
{
   if (getRemainingTime() > 0.005f)
   {
      return;
   }

   switch (_shake_type)
   {
      case ShakeType::Sine:
      {
         _shake_function = std::bind(&BoomEffect::shakeSine, this);
         break;
      }
      case ShakeType::Perlin:
      {
         break;
      }
   }

   _factor_x = x;
   _factor_y = y;
   _boom_factor = factor;
   _boom_time_end = GlobalClock::getInstance().getElapsedTime() + sf::seconds(_boom_duration);
}


//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::getRemainingTime() const
{
   return (_boom_time_end - GlobalClock::getInstance().getElapsedTime()).asSeconds();
}

//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::shakeSine() const
{
   const auto remaining_time = getRemainingTime();
   const auto time_normalized = (_boom_duration - remaining_time) / _boom_duration;
   const auto time_with_velocity = time_normalized * _effect_velocity;
   const auto time_with_velocity_square = time_with_velocity * time_with_velocity;
   const auto y = _boom_factor * _effect_amplitude * 2.0f * sin(time_with_velocity_square) * (1.0f / (1.0f + time_with_velocity_square));
   return y;
}

//----------------------------------------------------------------------------------------------------------------------
float BoomEffect::shakePerlin() const
{
   // 2 * (noise(x * 4) * (3 - x))
   // amplitude (noise(x * frequency) * (overall_duration - x))
   // https://graphtoy.com/?f1(x,t)=2%20*%20(noise(x%20*%204)%20*%20(3%20-%20x))&v1=true
   return 0.0f;
}

//----------------------------------------------------------------------------------------------------------------------
void BoomEffect::update(const sf::Time& /*dt*/)
{
   if (getRemainingTime() > 0.0f)
   {
      const auto& game_config = GameConfiguration::getInstance();
      const auto y = _shake_function();

      _boom_offset_x = _factor_x * game_config._view_width * y;
      _boom_offset_y = _factor_y * game_config._view_height * y;
   }
   else
   {
      _boom_offset_x = 0;
      _boom_offset_y = 0;
   }
}

