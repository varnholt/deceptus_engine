#ifndef BOOMEFFECTENVELOPE_H
#define BOOMEFFECTENVELOPE_H

#include <functional>
#include "boomsettings.h"

/// \brief defines the envelope interface used to shape camera shake over normalized time.
class BoomEffectEnvelope
{
public:
   BoomEffectEnvelope() = default;
   /// \brief destroys the envelope instance through the base interface.
   virtual ~BoomEffectEnvelope() = default;

   /// \brief evaluates shake amplitude for the given normalized time.
   /// \param t normalized elapsed time in the range [0.0, 1.0] during a shake.
   /// \return envelope output used to scale camera offset for the current frame.
   virtual float shakeFunction(float t) = 0;

   BoomSettings _settings;
};

#endif  // BOOMEFFECTENVELOPE_H
