#ifndef BOUNCERWRAPPER_H
#define BOUNCERWRAPPER_H

#include <memory>

class Bouncer;

namespace BouncerWrapper
{
/// \brief returns the first bouncer that currently reports the player in range.
/// \return nearby bouncer instance, or nullptr when none is in range.
std::shared_ptr<Bouncer> getNearbyBouncer();

/// \brief records the current time as the latest bouncer usage.
void bumpLastBouncerTime();

/// \brief checks whether enough time has passed since last bounce to stop temporary speed capping.
/// \return true when the cooldown window has already elapsed.
bool isSpeedCapped();
};

#endif  // BOUNCERWRAPPER_H
