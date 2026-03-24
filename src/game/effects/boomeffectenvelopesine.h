#ifndef BOOMEFFECTENVELOPESINE_H
#define BOOMEFFECTENVELOPESINE_H

#include "boomeffectenvelope.h"

/// \brief generates a smooth oscillating shake envelope with damped sine motion.
class BoomEffectEnvelopeSine : public BoomEffectEnvelope
{
public:
   BoomEffectEnvelopeSine() = default;
   /// \brief evaluates a sine-based waveform that fades out as normalized time increases.
   /// \param t normalized elapsed time in the active shake interval.
   /// \return signed shake amplitude scaled by configured envelope amplitude.
   float shakeFunction(float t) override;
};

#endif  // BOOMEFFECTENVELOPESINE_H
