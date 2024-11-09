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

void Weapon::update(const WeaponUpdateData& /*data*/)
{
}

int32_t Weapon::getDamage() const
{
   return 0;
}

void Weapon::setParentAudioUpdateData(const AudioUpdateData& parent_audio_update_data)
{
   _parent_audio_update_data = parent_audio_update_data;
}

void Weapon::initialize()
{
}
