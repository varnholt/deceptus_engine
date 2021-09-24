#pragma once

#include <SFML/Graphics.hpp>


struct BoomEffect
{
   void boom(float x, float y, float intensity = 1.0f);
   void update(const sf::Time& dt);
   float getRemainingTime() const;

   sf::Time _boom_time_end;
   float _boom_factor = 1.0f;
   float _boom_offset_x = 0.0f;
   float _boom_offset_y = 0.0f;
   float _boom_duration = 1.0f;
   float _factor_x = 0.0f;
   float _factor_y = 0.0f;
   float _effect_velocity = 32.0f;
   float _effect_amplitude = 0.1f;
};

