#ifndef PORTALWRAPPER_H
#define PORTALWRAPPER_H

#include <memory>

class Portal;

namespace PortalWrapper
{
std::shared_ptr<Portal> getNearbyPortal();
};

#endif // PORTALWRAPPER_H
