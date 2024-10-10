#include "weaponfactory.h"

#include <utility>

#include "bow.h"
#include "gun.h"
#include "sword.h"

std::shared_ptr<Weapon> WeaponFactory::create(WeaponType type)
{
   std::shared_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Bow:
      {
         weapon = std::make_shared<Bow>();
         break;
      }
      case WeaponType::Gun:
      {
         weapon = std::make_shared<Gun>();
         break;
      }
      case WeaponType::Sword:
      {
         weapon = std::make_shared<Sword>();
         break;
      }
      case WeaponType::None:
      {
         std::unreachable();
         break;
      }
   }

   weapon->initialize();

   return weapon;
}

std::shared_ptr<Weapon> WeaponFactory::create(WeaponType type, const WeaponProperties& properties)
{
   std::shared_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Bow:
      {
         weapon = std::make_shared<Bow>(properties);
         break;
      }
      case WeaponType::Gun:
      {
         weapon = std::make_shared<Gun>(properties);
         break;
      }
      case WeaponType::Sword:
      {
         weapon = std::make_shared<Sword>();
         break;
      }
      case WeaponType::None:
      {
         std::unreachable();
         break;
      }
   }

   weapon->initialize();

   return weapon;
}
