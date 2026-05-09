#ifndef EXTRAWRAPPER_H
#define EXTRAWRAPPER_H

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

namespace ExtraWrapper
{
/// \brief finds an extra by object id in the current level and triggers its spawn.
/// \param id object id of the extra to spawn.
/// \param offset pixel offset applied to the extra's position before it becomes collectable.
void spawnExtra(const std::string& id, sf::Vector2f offset = {});
};  // namespace ExtraWrapper

#endif  // EXTRAWRAPPER_H
