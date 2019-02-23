#pragma once

// sfml
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

// box2d
#include <Box2D/Box2D.h>

// std
#include <list>
#include <memory>
#include <set>

// game
#include "game/bullet.h"


class Weapon
{
public:

   enum class WeaponType
   {
      Slingshot,
      Pistol,
      Bazooka,
      Laser,
      Aliengun
   };

   Weapon();
   Weapon(std::unique_ptr<b2Shape>, int fireInterval);

   void fire(
      b2World* world,
      const b2Vec2 &pos,
      const b2Vec2 &dir
   );

   int getFireInterval() const;
   void setFireInterval(int interval);

   void drawBullets(sf::RenderTarget& target);

   // later on each weapon shall have its own animation
   static void updateBulletHitAnimations(float dt);
   static void drawBulletHits(sf::RenderTarget& target);

   int damage() const;
   void loadTextures();


protected:

   static void cleanupBullets();

   static std::set<Bullet*> sBullets;
   static std::list<b2Vec2> sDetonationPositions;

   WeaponType mType;

   sf::Texture mBulletTexture;
   sf::Sprite mBulletSprite;

   std::unique_ptr<b2Shape> mShape;
   sf::Clock mFireClock;
   int mFireInterval = 100;
};
