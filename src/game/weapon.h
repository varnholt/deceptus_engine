#pragma once

#include <SFML/Graphics.hpp>

#include "audioupdatedata.h"
#include "constants.h"

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

   // todo
   // make virtual and move to subclasses
   int32_t damage() const;

   void setParentAudioUpdateData(const AudioUpdateData& parent_audio_update_data);

protected:
   WeaponType _type = WeaponType::None;
   std::optional<AudioUpdateData> _parent_audio_update_data;
};
