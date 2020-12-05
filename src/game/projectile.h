#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "fixturenode.h"
#include "gamenode.h"
#include "projectilehitanimation.h"

#include <functional>
#include <list>
#include <map>
#include <set>

class b2Body;

class Projectile : public FixtureNode
{

public:

   struct HitInformation
   {
      b2Vec2 _pos;
      WeaponType _weapon_type = WeaponType::Default;
   };

   using DestroyedCallback = std::function<void(void)>;

   Projectile();
   virtual ~Projectile();

   b2Body *getBody() const;
   void setBody(b2Body *body);

   static void clear();
   static void cleanup();
   static void updateHitAnimations(const sf::Time& dt);

   void addDestroyedCallback(const DestroyedCallback& destroyedCallback);

   bool isSticky() const;
   void setSticky(bool sticky);

   bool hitSomething() const;
   void setHitSomething(bool hit_something);

   bool isScheduledForRemoval() const;
   void setScheduledForRemoval(bool isScheduledForRemoval);

   bool isScheduledForInactivity() const;
   void setScheduledForInactivity(bool scheduled_for_inactivity);


protected:

   bool _scheduled_for_removal = false;
   bool _scheduled_for_inactivity = false;
   bool _sticky = false;
   bool _hit_something = false;
   b2Body* _body = nullptr;
   WeaponType _weapon_type = WeaponType::Default;
   std::vector<DestroyedCallback> _destroyed_callbacks;

   static std::map<WeaponType, ProjectileHitAnimation::FrameData> _hit_animations;
   static std::set<Projectile*> _projectiles;
   static std::list<HitInformation> _hit_positions;
};


