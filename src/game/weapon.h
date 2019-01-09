#pragma once

// sfml
#include <SFML/Graphics/Vertex.hpp>
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

   static std::set<Bullet*> sBullets;
   static std::list<b2Vec2> sDetonationPositions;


protected:

   std::unique_ptr<b2Shape> mShape;
   sf::Clock mFireClock;
   int mFireInterval = 100;


public:

   Weapon();
   Weapon(std::unique_ptr<b2Shape>, int fireInterval);

   void fire(
      b2World* world,
      const b2Vec2 &pos,
      const b2Vec2 &dir
   );

   int getFireInterval() const;
   void setFireInterval(int interval);

   static void cleanupBullets();

};
