#include "weaponfactory.h"

#include "bow.h"

std::unique_ptr<Weapon> WeaponFactory::create(WeaponType type)
{
   std::unique_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Default:
      {
         weapon = std::make_unique<Weapon>();
      }
      case WeaponType::Bow:
      {
         weapon = std::make_unique<Bow>();
      }
   }

   return std::move(weapon);
}


std::unique_ptr<Weapon> WeaponFactory::create(
   b2Body* parent_body,
   WeaponType type,
   std::unique_ptr<b2Shape> shape,
   int32_t fireInterval,
   int32_t damage
)
{
   std::unique_ptr<Weapon> weapon;

   switch (type)
   {
      case WeaponType::Default:
      {
         weapon = std::make_unique<Weapon>(std::move(shape), fireInterval, damage);
         break;
      }
      case WeaponType::Bow:
      {
         auto tmp = std::make_unique<Bow>();
         tmp->setFireIntervalMs(fireInterval);
         tmp->setLauncherBody(parent_body);
         weapon = std::move(tmp);
         break;
      }
   }

   return std::move(weapon);
}

