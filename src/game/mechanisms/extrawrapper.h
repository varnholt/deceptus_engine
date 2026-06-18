#ifndef EXTRAWRAPPER_H
#define EXTRAWRAPPER_H

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <string>

class Extra;

namespace ExtraWrapper
{
/// \brief finds an extra by object id in the current level and triggers its spawn.
/// \param id object id of the extra to spawn.
/// \param offset pixel offset applied to the extra's position before it becomes collectable.
/// \return shared pointer to the spawned extra, or nullptr when the id is not found.
std::shared_ptr<Extra> spawnExtra(const std::string& id, sf::Vector2f offset = {});
};  // namespace ExtraWrapper

#endif  // EXTRAWRAPPER_H
