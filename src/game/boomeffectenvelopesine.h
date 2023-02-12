#ifndef BOOMEFFECTENVELOPESINE_H
#define BOOMEFFECTENVELOPESINE_H

#include "boomeffectenvelope.h"

class BoomEffectEnvelopeSine : public BoomEffectEnvelope
{
public:
   BoomEffectEnvelopeSine() = default;
   float shakeFunction(float t) override;
};

#endif // BOOMEFFECTENVELOPESINE_H
