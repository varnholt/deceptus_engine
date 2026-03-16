#include "weaponfactory.h"

#include <unordered_map>
#include <utility>

#include "bow.h"
#include "gun.h"
#include "playersword.h"

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
         weapon = std::make_shared<PlayerSword>();
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
         weapon = std::make_shared<PlayerSword>();
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

std::shared_ptr<Weapon> WeaponFactory::create(const std::string& name)
{
   static const std::unordered_map<std::string, WeaponType> weapon_map = {
      {"Sword", WeaponType::Sword},
      {"Bow", WeaponType::Bow},
      {"Gun", WeaponType::Gun}
   };

   if (auto it = weapon_map.find(name); it != weapon_map.end())
   {
      return create(it->second);
   }

   return nullptr;
}
