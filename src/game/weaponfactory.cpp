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
   }

   return std::move(weapon);
}


std::unique_ptr<Weapon> WeaponFactory::create(
   b2Body* parent_body,
   WeaponType type,
   std::unique_ptr<b2Shape> shape,
   int32_t fire_interval,
   int32_t damage
)
{
   std::unique_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Bow:
      {
         auto bow = std::make_unique<Bow>();
         bow->setUseIntervalMs(fire_interval);
         bow->setLauncherBody(parent_body);
         weapon = std::move(bow);
         break;
      }
      case WeaponType::Gun:
      {
         weapon = std::make_unique<Gun>(std::move(shape), fire_interval, damage);
         break;
      }
      case WeaponType::Sword:
      {
         auto sword = std::make_unique<Sword>();
         weapon = std::move(sword);
         break;
      }
   }

   return std::move(weapon);
}

