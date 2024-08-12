#pragma once

#include <box2d/box2d.h>

#include "weapon.h"
#include "weaponproperties.h"

namespace WeaponFactory
{

std::shared_ptr<Weapon> create(WeaponType type);
std::shared_ptr<Weapon> create(WeaponType type, const WeaponProperties& properties);
}  // namespace WeaponFactory
