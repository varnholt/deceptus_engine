#pragma once

#include <SFML/Graphics.hpp>

#include "game/audio/audioupdatedata.h"
#include "game/constants.h"

#include <optional>

class Weapon
{
public:
   Weapon() = default;
   virtual ~Weapon() = default;
   WeaponType getWeaponType() const;

   virtual void draw(sf::RenderTarget& target);
   virtual void update(const sf::Time& time);
   virtual void initialize();
   virtual int32_t getDamage() const;
   virtual std::string getName() const = 0;

   void setParentAudioUpdateData(const AudioUpdateData& parent_audio_update_data);

protected:
   WeaponType _type = WeaponType::None;
   std::optional<AudioUpdateData> _parent_audio_update_data;
};
