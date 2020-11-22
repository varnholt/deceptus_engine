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

   enum class WeaponType
   {
      Bow,
      Slingshot,
      Pistol,
      Bazooka,
      Laser,
      Aliengun
   };

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

   int getFireInterval() const;
   void setFireInterval(int interval);

   void drawProjectiles(sf::RenderTarget& target);

   static void drawProjectileHits(sf::RenderTarget& target);

   int damage() const;
   void loadTextures();
   void setTexture(const std::filesystem::path& path, const sf::Rect<int32_t>& textureRect = mEmptyRect);


protected:

   void cleanupProjectiles();

   std::set<Projectile*> mProjectiles;

   WeaponType mType;

   sf::Texture mProjectileTexture;
   sf::Sprite mProjectileSprite;

   std::unique_ptr<b2Shape> mShape;

   sf::Clock mFireClock;
   int32_t mFireInterval = 100;
   int32_t mDamage = 100;
   std::filesystem::path mTexturePath = "data/weapons/bullet.png";
   sf::Rect<int32_t> mTextureRect;

   static sf::Rect<int32_t> mEmptyRect;
};
