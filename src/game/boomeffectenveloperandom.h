#ifndef BOOMEFFECTENVELOPERANDOM_H
#define BOOMEFFECTENVELOPERANDOM_H

#include "boomeffectenvelope.h"

#include "framework/math/fbm.h"

class BoomEffectEnvelopeRandom : public BoomEffectEnvelope
{
public:
   BoomEffectEnvelopeRandom() = default;

   float shakeFunction(float t) override;

   fbm::vec2 _st;
};

#endif // BOOMEFFECTENVELOPERANDOM_H
