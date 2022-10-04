#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <memory>

#include "boomeffectenvelope.h"

struct BoomEffect
{
   enum class ShakeType
   {
      Sine,
      Random
   };

   void boom(float x, float y, float amplitude = 1.0f, float boom_duration = 1.0f, ShakeType shake_type = ShakeType::Random);
   void update(const sf::Time& dt);

   float getRemainingTime() const;
   float getRemainingTimeNormalized() const;

   sf::Time _boom_time_end;
   float _boom_offset_x = 0.0f;
   float _boom_offset_y = 0.0f;
   float _boom_duration = 1.0f;
   float _factor_x = 0.0f;
   float _factor_y = 0.0f;

   std::shared_ptr<BoomEffectEnvelope> _envelope;

   using ShakeFunction = std::function<float(float)>;
   ShakeFunction _shake_function;
};
