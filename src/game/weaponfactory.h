#pragma once

#include "weapon.h"

namespace WeaponFactory
{

std::unique_ptr<Weapon> create(WeaponType type);
std::unique_ptr<Weapon> create(
   b2Body* parent_body,
   WeaponType type,
   std::unique_ptr<b2Shape>,
   int32_t fireInterval,
   int32_t damage
);

};

