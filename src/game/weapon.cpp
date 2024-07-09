// base
#include "weapon.h"

// game
#include "constants.h"

WeaponType Weapon::getWeaponType() const
{
   return _type;
}

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
      case WeaponType::None:
         break;
   }

   return damage_value;
}

void Weapon::setParentAudioUpdateData(const AudioUpdateData& parent_audio_update_data)
{
   _parent_audio_update_data = parent_audio_update_data;
}

void Weapon::initialize()
{
}
