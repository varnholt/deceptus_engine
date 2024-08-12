#ifndef BOOMEFFECTENVELOPE_H
#define BOOMEFFECTENVELOPE_H

#include <functional>
#include "boomsettings.h"

class BoomEffectEnvelope
{
public:
   BoomEffectEnvelope() = default;
   virtual ~BoomEffectEnvelope() = default;

   virtual float shakeFunction(float t) = 0;

   BoomSettings _settings;
};

#endif  // BOOMEFFECTENVELOPE_H
