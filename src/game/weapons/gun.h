#pragma once

#include "game/weapons/weapon.h"

// sfml
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

// box2d
#include "box2d/box2d.h"

// std
#include <filesystem>
#include <memory>

// game
#include "game/weapons/projectile.h"
#include "game/weapons/weaponproperties.h"

/// \brief generic projectile weapon that spawns, updates, and draws bullet-like projectiles.
class Gun : public Weapon
{
public:
   /// \brief stores shared animation and identifier data copied into spawned projectiles.
   struct ProjectileAnimation
   {
      std::optional<std::string> _identifier;
      std::filesystem::path _texture_path = "data/weapons/bullet.png";
      Animation _animation;
      AnimationFrameData _frame_data;
   };

   /// \brief constructs a default gun with circular projectile shape and default animation.
   Gun();
   /// \brief constructs a gun configured from weapon properties.
   /// \param properties configuration source for cooldown, damage, physics, and projectile shape.
   Gun(const WeaponProperties& properties);

   /// \brief fires only when the cooldown interval has elapsed.
   /// \param world box2d world that receives newly spawned projectile bodies.
   /// \param pos spawn position in box2d world units.
   /// \param dir impulse vector applied to the spawned projectile body.
   virtual void useInIntervals(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir);
   /// \brief spawns one projectile body, assigns visuals, and stores it for updates and rendering.
   /// \param world box2d world that receives the projectile body.
   /// \param pos spawn position in box2d world units.
   /// \param dir impulse vector applied immediately after spawn.
   virtual void use(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir);

   /// \brief draws all active projectile animations.
   /// \param target render target used for projectile rendering.
   void draw(sf::RenderTarget& target) override;
   /// \brief updates projectile animations and applies deferred inactivity to physics bodies.
   /// \param time per-frame update context containing delta time and world.
   void update(const WeaponUpdateData& time) override;
   /// \brief returns nominal gun damage value.
   /// \return damage amount used by default gun setup.
   int32_t getDamage() const override;
   /// \brief returns the weapon name used by gameplay and config code.
   /// \return string literal "gun".
   std::string getName() const override;

   /// \brief draws globally managed projectile hit animations.
   /// \param target render target used for hit effect rendering.
   static void drawProjectileHitAnimations(sf::RenderTarget& target);
   /// \brief builds a single-frame reference animation from a texture region.
   /// \param texture texture used by projectile visuals.
   /// \param textureRect optional source rectangle in pixels; full texture is used when empty.
   void setProjectileAnimation(const std::shared_ptr<sf::Texture>& texture, const sf::Rect<int32_t>& textureRect = _empty_rect);
   /// \brief copies multi-frame animation data as the projectile reference animation.
   /// \param frame_data frame data used for all newly spawned projectiles.
   void setProjectileAnimation(const AnimationFrameData& frame_data);

   /// \brief returns the minimum delay between two allowed shots.
   /// \return cooldown interval in milliseconds.
   int32_t getUseIntervalMs() const;
   /// \brief sets the minimum delay between two allowed shots.
   /// \param interval cooldown interval in milliseconds.
   void setUseIntervalMs(int32_t interval);

   /// \brief returns the optional identifier used for hit animation and hit audio lookup.
   /// \return optional projectile identifier set on this weapon.
   std::optional<std::string> getProjectileIdentifier() const;
   /// \brief sets the projectile identifier propagated to newly spawned projectiles.
   /// \param projectile_identifier identifier key for projectile hit resources.
   void setProjectileIdentifier(const std::string& projectile_identifier);

protected:
   /// \brief draws projectile animations stored by this weapon instance.
   /// \param target render target used for projectile rendering.
   void drawProjectiles(sf::RenderTarget& target);
   /// \brief updates animation transform and frame state for each projectile.
   /// \param time frame delta time used by animation update.
   void updateProjectiles(const sf::Time& time);
   /// \brief clones the reference animation into a projectile instance and starts playback.
   /// \param projectile projectile receiving the copied animation state.
   void copyReferenceAnimation(Projectile* projectile);

   std::vector<Projectile*> _projectiles;
   ProjectileAnimation _projectile_reference_animation;
   std::unique_ptr<b2Shape> _shape;
   sf::Clock _fire_clock;

   int32_t _use_interval_ms{100};
   int32_t _damage{100};
   float _gravity_scale{0.0f};
   float _density{1.0f};

   static sf::Rect<int32_t> _empty_rect;
};
