#pragma once

#include "Box2D/Box2D.h"

#include "weapon.h"
#include "weaponproperties.h"

namespace WeaponFactory
{

std::unique_ptr<Weapon> create(WeaponType type);
std::unique_ptr<Weapon> create(WeaponType type, const WeaponProperties& properties);
}  // namespace WeaponFactory
