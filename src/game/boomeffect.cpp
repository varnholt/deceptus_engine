#include "boomeffect.h"

#include "camerasystemconfiguration.h"
#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"

#include <math.h>
#include <iostream>

#include "boomeffectenveloperandom.h"
#include "boomeffectenvelopesine.h"

BoomSettings BoomEffect::_default_boom_settings = BoomSettings{1.0f, 1.0f};

//-----------------------------------------------------------------------------
void BoomEffect::boom(float x, float y, const BoomSettings& boom_settings)
{
   if (!CameraSystemConfiguration::getInstance()._camera_shaking_enabled)
   {
      return;
   }

   if (getRemainingTime() > 0.005f)
   {
      return;
   }

   switch (boom_settings._shake_type)
   {
      case BoomSettings::ShakeType::Sine:
      {
         _envelope = std::make_shared<BoomEffectEnvelopeSine>();
         break;
      }
      case BoomSettings::ShakeType::Random:
      {
         _envelope = std::make_shared<BoomEffectEnvelopeRandom>();
         break;
      }
   }

   _shake_function = [this](float x) -> float { return _envelope->shakeFunction(x); };
   _envelope->_settings = boom_settings;

   _factor_x = x;
   _factor_y = y;

   _boom_duration_s = boom_settings._boom_duration_s;
   _boom_time_end = GlobalClock::getInstance().getElapsedTime() + sf::seconds(_boom_duration_s);
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
   const auto time_normalized = (_boom_duration_s - remaining_time) / _boom_duration_s;
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
