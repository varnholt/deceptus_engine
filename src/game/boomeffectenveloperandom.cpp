#include "boomeffectenveloperandom.h"

#include <iostream>

BoomEffectEnvelopeRandom::BoomEffectEnvelopeRandom()
{
   _effect_frequency = 0.1f;
   _effect_amplitude = 5.0f;
}

float BoomEffectEnvelopeRandom::shakeFunction(float t)
{
   // 2 * (noise(x * 4) * (3 - x))
   // amplitude (noise(x * frequency) * (overall_duration - x))
   // https://graphtoy.com/?f1(x,t)=2%20*%20(noise(x%20*%204)%20*%20(3%20-%20x))&v1=true

   _st.x += _effect_frequency;

   const auto noise = fbm::fbm(_st) - 0.5f;
   const auto y = _effect_amplitude * (noise * (1.0f - t));

   // std::cout << y << std::endl;

   return y;
}

