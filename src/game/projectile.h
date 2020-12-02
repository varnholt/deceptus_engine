#pragma once

// sfml
#include <SFML/Graphics.hpp>

// box2d
#include <Box2D/Box2D.h>

// base
#include "gamenode.h"
#include "fixturenode.h"

#include <functional>
#include <list>
#include <set>

class b2Body;

class Projectile : public FixtureNode
{

public:

   using DestroyedCallback = std::function<void(void)>;

   Projectile();
   virtual ~Projectile();

   bool isScheduledForRemoval() const;
   void setScheduledForRemoval(bool isScheduledForRemoval);

   b2Body *getBody() const;
   void setBody(b2Body *body);

   static void clear();
   static void cleanup();
   static void updateHitAnimations(const sf::Time& dt);

   void setDestroyedCallback(const DestroyedCallback& destroyedCallback);

   bool isSticky() const;
   void setSticky(bool sticky);

   bool hitSomething() const;
   void setHitSomething(bool hit_something);


   protected:

   bool _scheduled_for_removal = false;
   b2Body* _body = nullptr;
   DestroyedCallback _destroyed_callback;
   bool _sticky = false;
   bool _hit_something = false;

   static std::set<Projectile*> _projectiles;
   static std::list<b2Vec2> _hit_positions;
};


