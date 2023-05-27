#pragma once

#include "weapon.h"

// sfml
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

// box2d
#include <Box2D/Box2D.h>

// std
#include <filesystem>
#include <memory>

// game
#include "game/projectile.h"
#include "weaponproperties.h"

class Gun : public Weapon
{
public:
   struct ProjectileAnimation
   {
      std::optional<std::string> _identifier;
      std::filesystem::path _texture_path = "data/weapons/bullet.png";
      Animation _animation;
      AnimationFrameData _frame_data;
   };

   Gun();
   Gun(const WeaponProperties& properties);

   virtual void useInIntervals(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir);
   virtual void use(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir);

   void draw(sf::RenderTarget& target) override;
   void update(const sf::Time& time) override;

   static void drawProjectileHitAnimations(sf::RenderTarget& target);
   void setProjectileAnimation(const std::shared_ptr<sf::Texture>& texture, const sf::Rect<int32_t>& textureRect = _empty_rect);
   void setProjectileAnimation(const AnimationFrameData& frame_data);

   int32_t getUseIntervalMs() const;
   void setUseIntervalMs(int32_t interval);

   std::optional<std::string> getProjectileIdentifier() const;
   void setProjectileIdentifier(const std::string& projectile_identifier);

protected:
   void drawProjectiles(sf::RenderTarget& target);
   void updateProjectiles(const sf::Time& time);
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
