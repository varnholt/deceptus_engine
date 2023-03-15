#include "weaponfactory.h"

#include "bow.h"
#include "gun.h"
#include "sword.h"


std::unique_ptr<Weapon> WeaponFactory::create(WeaponType type)
{
   std::unique_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Bow:
      {
         weapon = std::make_unique<Bow>();
         break;
      }
      case WeaponType::Gun:
      {
         weapon = std::make_unique<Gun>();
         break;
      }
      case WeaponType::Sword:
      {
         weapon = std::make_unique<Sword>();
         break;
      }
      case WeaponType::None:
      {
         break;
      }
   }

   return weapon;
}

std::unique_ptr<Weapon> WeaponFactory::create(WeaponType type, const WeaponProperties& properties)
{
   std::unique_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Bow:
      {
         auto bow = std::make_unique<Bow>();

         // todo: move weaponproperties into constructor
         bow->setUseIntervalMs(properties._fire_interval_ms);
         bow->setLauncherBody(properties._parent_body);
         weapon = std::move(bow);
         break;
      }
      case WeaponType::Gun:
      {
         weapon = std::make_unique<Gun>(properties);
         break;
      }
      case WeaponType::Sword:
      {
         auto sword = std::make_unique<Sword>();
         weapon = std::move(sword);
         break;
      }
      case WeaponType::None:
      {
         break;
      }
   }

   return weapon;
}

