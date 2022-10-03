#ifndef BOOMEFFECTENVELOPERANDOM_H
#define BOOMEFFECTENVELOPERANDOM_H

#include "boomeffectenvelope.h"

class BoomEffectEnvelopeRandom : public BoomEffectEnvelope
{
public:
   BoomEffectEnvelopeRandom();

   float shakeFunction(float t);
};

#endif // BOOMEFFECTENVELOPERANDOM_H
