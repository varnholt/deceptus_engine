#pragma once

#include <SFML/Graphics.hpp>

#include "game/audio/audioupdatedata.h"
#include "game/constants.h"

#include "box2d/box2d.h"

#include <memory>
#include <optional>

/// \brief abstract base interface for all player and enemy weapons.
class Weapon
{
public:
   /// \brief per-frame context passed to weapon update methods.
   struct WeaponUpdateData
   {
      const sf::Time& _time;
      std::shared_ptr<b2World> _world;
   };

   /// \brief constructs a weapon with type set to none.
   Weapon() = default;

   /// \brief virtual destructor for polymorphic cleanup.
   virtual ~Weapon() = default;

   /// \brief returns the runtime weapon type tag.
   /// \return weapon type enum value.
   WeaponType getWeaponType() const;

   /// \brief draws weapon-specific visuals.
   /// \param target render target used for weapon rendering.
   virtual void draw(sf::RenderTarget& target);

   /// \brief updates weapon state for the current frame.
   /// \param data per-frame timing and world context.
   virtual void update(const WeaponUpdateData& data);

   /// \brief performs optional post-construction initialization.
   virtual void initialize();

   /// \brief returns nominal damage dealt by this weapon.
   /// \return damage value used by weapon hits.
   virtual int32_t getDamage() const;

   /// \brief returns a stable weapon name string.
   /// \return weapon name used for lookup and debugging.
   virtual std::string getName() const = 0;

   /// \brief stores parent audio context that can be propagated to spawned projectiles.
   /// \param parent_audio_update_data audio update data provided by the owning actor.
   void setParentAudioUpdateData(const AudioUpdateData& parent_audio_update_data);

protected:
   WeaponType _type = WeaponType::None;
   std::optional<AudioUpdateData> _parent_audio_update_data;
};
