// base
#include "weapon.h"

// game
#include "constants.h"
#include "projectile.h"
#include "projectilehitanimation.h"
#include "texturepool.h"

#include <iostream>



void Weapon::draw(sf::RenderTarget& /*target*/)
{
}


void Weapon::update(const sf::Time& /*time*/)
{
}


int32_t Weapon::damage() const
{
   auto damage_value = 0;

   switch (_type)
   {
      case WeaponType::Bow:
         damage_value = 20;
         break;
      case WeaponType::Gun:
         damage_value = 20;
         break;
      case WeaponType::Sword:
         damage_value = 20;
         break;
   }

   return damage_value;
}


void Weapon::initialize()
{
}


