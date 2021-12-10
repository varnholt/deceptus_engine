#pragma once

#include "Box2D/Box2D.h"

#include "weapon.h"

namespace WeaponFactory
{

std::unique_ptr<Weapon> create(WeaponType type);
std::unique_ptr<Weapon> create(
   b2Body* parent_body,
   WeaponType type,
   std::unique_ptr<b2Shape>,
   int32_t use_interval,
   int32_t damage
);

}

