#include "boomeffectenvelopesine.h"

//   // remove/move to envelopes
//   float _effect_frequency = 32.0f;
//   float _effect_amplitude = 0.1f;

float BoomEffectEnvelopeSine::shakeFunction(float t)
{
   const auto time_with_velocity = t * _effect_frequency;
   const auto time_with_velocity_square = time_with_velocity * time_with_velocity;
   const auto y =
      _effect_amplitude * _effect_amplitude * 2.0f * sin(time_with_velocity_square) * (1.0f / (1.0f + time_with_velocity_square));
   return y;
}
