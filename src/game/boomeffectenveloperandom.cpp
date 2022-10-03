#include "boomeffectenveloperandom.h"

BoomEffectEnvelopeRandom::BoomEffectEnvelopeRandom()
{
}

float BoomEffectEnvelopeRandom::shakeFunction(float t)
{
   return t;
}

//----------------------------------------------------------------------------------------------------------------------
// float BoomEffect::shakeRandom() const
//{
//   // 2 * (noise(x * 4) * (3 - x))
//   // amplitude (noise(x * frequency) * (overall_duration - x))
//   // https://graphtoy.com/?f1(x,t)=2%20*%20(noise(x%20*%204)%20*%20(3%20-%20x))&v1=true
//   return 0.0f;
//}
