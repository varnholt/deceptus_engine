#ifndef BOUNCERWRAPPER_H
#define BOUNCERWRAPPER_H

#include <memory>

class Bouncer;

namespace BouncerWrapper
{
std::shared_ptr<Bouncer> getNearbyBouncer();
void bumpLastBouncerTime();
bool isSpeedCapped();
};

#endif  // BOUNCERWRAPPER_H
