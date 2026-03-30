#ifndef BOOMEFFECTENVELOPERANDOM_H
#define BOOMEFFECTENVELOPERANDOM_H

#include "boomeffectenvelope.h"

#include "framework/math/fbm.h"

/// \brief generates a noise-driven shake envelope that decays as time approaches the end.
class BoomEffectEnvelopeRandom : public BoomEffectEnvelope
{
public:
   BoomEffectEnvelopeRandom() = default;

   /// \brief samples fractal noise and attenuates it linearly over normalized time.
   /// \param t normalized elapsed time in the active shake interval.
   /// \return signed shake amplitude scaled by envelope amplitude and remaining lifetime.
   float shakeFunction(float t) override;

   fbm::vec2 _st;
};

#endif  // BOOMEFFECTENVELOPERANDOM_H
