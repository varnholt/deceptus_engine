#pragma once

#include <vector>

#include "weapon.h"
#include "Box2D/Box2D.h"

class Bow : public Weapon
{
   struct ArrowCollision
   {
      b2Body* _arrow = nullptr;
      b2Body* _target = nullptr;
   };


public:

   Bow() = default;

   void load(b2World* world);
   void fireNow(
      const std::shared_ptr<b2World>& world,
      const b2Vec2& pos,
      const b2Vec2& dir
   );

   void update();

   void postSolve(b2Contact* contact, const b2ContactImpulse* impulse);

   b2Body* getLauncherBody() const;
   void setLauncherBody(b2Body* launcher_body);


private:

   b2Body* _loaded_arrow_body = nullptr;
   b2Body* _launcher_body = nullptr;
   std::vector<b2Body*> _arrow_bodies;
   std::vector<ArrowCollision> _arrow_collisions;
};



