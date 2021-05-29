#pragma once

#include <vector>

#include "weapon.h"
#include "Box2D/Box2D.h"


struct Arrow;

class Bow : public Weapon
{

public:

   Bow();
   virtual ~Bow();

   void load(b2World* world);
   void fireNow(
      const std::shared_ptr<b2World>& world,
      const b2Vec2& pos,
      const b2Vec2& dir
   ) override;

   void update(const sf::Time& /*time*/) override;

   b2Body* getLauncherBody() const;
   void setLauncherBody(b2Body* launcher_body);


private:

   void updateRotation(Arrow*);
   Arrow* _loaded_arrow = nullptr;
   b2Body* _launcher_body = nullptr;
   std::vector<Arrow*> _arrows;
};



