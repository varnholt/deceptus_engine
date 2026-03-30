#ifndef EXTRAWRAPPER_H
#define EXTRAWRAPPER_H

#include <string>

namespace ExtraWrapper
{
/// \brief finds an extra by object id in the current level and triggers its spawn.
/// \param id object id of the extra to spawn.
void spawnExtra(const std::string& id);
};

#endif  // EXTRAWRAPPER_H
