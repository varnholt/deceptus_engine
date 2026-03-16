#pragma once

#include "box2d/box2d.h"

#include "weapon.h"
#include "weaponproperties.h"

#include <memory>
#include <string>

namespace WeaponFactory
{

std::shared_ptr<Weapon> create(WeaponType type);
std::shared_ptr<Weapon> create(WeaponType type, const WeaponProperties& properties);
std::shared_ptr<Weapon> create(const std::string& name);
}  // namespace WeaponFactory
