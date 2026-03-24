#pragma once

#include "box2d/box2d.h"

#include "weapon.h"
#include "weaponproperties.h"

#include <memory>
#include <string>

namespace WeaponFactory
{

/// \brief creates and initializes a weapon instance for a given weapon type.
/// \param type weapon type to instantiate.
/// \return shared weapon instance, or nullptr when creation fails.
std::shared_ptr<Weapon> create(WeaponType type);

/// \brief creates and initializes a weapon using explicit runtime properties.
/// \param type weapon type to instantiate.
/// \param properties properties forwarded to type-specific constructors when supported.
/// \return shared weapon instance, or nullptr when creation fails.
std::shared_ptr<Weapon> create(WeaponType type, const WeaponProperties& properties);

/// \brief creates and initializes a weapon from a human-readable type name.
/// \param name weapon name such as "Sword", "Bow", or "Gun".
/// \return shared weapon instance, or nullptr when the name is unknown.
std::shared_ptr<Weapon> create(const std::string& name);
}  // namespace WeaponFactory
