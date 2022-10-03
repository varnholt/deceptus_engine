#ifndef BOOMEFFECTENVELOPE_H
#define BOOMEFFECTENVELOPE_H

#include <functional>

class BoomEffectEnvelope
{
public:
   BoomEffectEnvelope() = default;
   virtual ~BoomEffectEnvelope() = default;

   virtual float shakeFunction(float t) = 0;

   float _effect_frequency = 1.0f;
   float _effect_amplitude = 1.0f;
};

#endif // BOOMEFFECTENVELOPE_H
