#pragma once

#include "weather.h"


struct TmxObject;

class ThunderstormOverlay : public WeatherOverlay
{

public:

   struct ThunderstormSettings{
      float _thunderstorm_time_s = 3.0;
      float _silence_time_s = 5.0f;
   };

   ThunderstormOverlay() = default;

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   void setRect(const sf::FloatRect& newRect);


   void setSettings(const ThunderstormSettings& newSettings);

   private:

   enum class State
   {
      Lightning,
      Silence
   };

   sf::Sprite _sprite;
   sf::FloatRect _rect;
   float _value = 0.0f;
   float _factor = 0.0f;
   float _time_s = 0.0f;
   float _thunderstorm_time_elapsed_s = 0.0;
   float _silence_time_elapsed_s = 0.0f;
   State _state = State::Silence;

   ThunderstormSettings _settings;
};

