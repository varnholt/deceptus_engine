#pragma once

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


class Weapon
{
public:

   Weapon();
   Weapon(std::unique_ptr<b2Shape>, int32_t fireInterval, int32_t damage);

   virtual void fireInIntervals(
      const std::shared_ptr<b2World>& world,
      const b2Vec2 &pos,
      const b2Vec2 &dir
   );

   virtual void fireNow(
      const std::shared_ptr<b2World>& world,
      const b2Vec2& pos,
      const b2Vec2& dir
   );

   virtual void draw(sf::RenderTarget& target);
   virtual void update(const sf::Time& time);

   virtual void initialize();
   virtual void loadTextures();

   static void drawProjectileHitAnimations(sf::RenderTarget& target);

   int damage() const;

   void setTexture(const std::filesystem::path& path, const sf::Rect<int32_t>& textureRect = _empty_rect);

   int getFireIntervalMs() const;
   void setFireIntervalMs(int interval);


protected:

   void drawProjectiles(sf::RenderTarget& target);

   std::vector<Projectile*> _projectiles;

   std::filesystem::path _texture_path = "data/weapons/bullet.png";
   std::shared_ptr<sf::Texture> _projectile_texture;
   sf::Rect<int32_t> _projectile_texture_rect;
   sf::Sprite _projectile_sprite;

   std::unique_ptr<b2Shape> _shape;

   sf::Clock _fire_clock;

   int32_t _fire_interval_ms = 100;
   int32_t _damage = 100;
   WeaponType _type = WeaponType::Default;

   static sf::Rect<int32_t> _empty_rect;
};
