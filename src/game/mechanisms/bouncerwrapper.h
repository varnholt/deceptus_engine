#ifndef BOUNCERWRAPPER_H
#define BOUNCERWRAPPER_H

#include <memory>

class Bouncer;

namespace BouncerWrapper
{
std::shared_ptr<Bouncer> getNearbyBouncer();
};

#endif // BOUNCERWRAPPER_H
