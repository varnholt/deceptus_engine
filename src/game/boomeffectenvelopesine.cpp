#include "boomeffectenvelopesine.h"

#include <math.h>

namespace
{
constexpr float frequency = 32.0f;
constexpr float internal_amplitude = 0.1f;
constexpr float internal_amplitude_2 = internal_amplitude * internal_amplitude;
}  // namespace

float BoomEffectEnvelopeSine::shakeFunction(float t)
{
   const auto time_with_freq = t * frequency;
   const auto time_with_freq_2 = time_with_freq * time_with_freq;
   const auto y = _settings._amplitude * internal_amplitude_2 * 2.0f * sin(time_with_freq_2) * (1.0f / (1.0f + time_with_freq_2));

   return y;
}
