#include "boomeffectenveloperandom.h"

#include <iostream>

namespace
{
constexpr float frequency = 0.1f;
}  // namespace

float BoomEffectEnvelopeRandom::shakeFunction(float t)
{
   // 2 * (noise(x * 4) * (3 - x))
   // amplitude (noise(x * frequency) * (overall_duration - x))
   // https://graphtoy.com/?f1(x,t)=2%20*%20(noise(x%20*%204)%20*%20(3%20-%20x))&v1=true

   _st.x += frequency;

   const auto noise = fbm::fbm(_st) - 0.5f;
   const auto y = _settings._amplitude * (noise * (1.0f - t));

   // std::cout << t << ": " << y << " (" << noise << ")" << std::endl;

   return y;
}
