#include "projectile.h"

#include "projectilehitanimation.h"

#include "Box2D/Box2D.h"

#include <memory>


std::list<b2Vec2> Projectile::_hit_positions;
std::set<Projectile*> Projectile::_projectiles;



Projectile::Projectile()
 : FixtureNode(this)
{
   setName(typeid(Projectile).name());
   _type = ObjectTypeProjectile;
   _projectiles.insert(this);
}


Projectile::~Projectile()
{
   _destroyed_callback();
   _body->GetWorld()->DestroyBody(_body);
}


bool Projectile::isScheduledForRemoval() const
{
   return _scheduled_for_removal;
}


void Projectile::setScheduledForRemoval(bool remove)
{
   _scheduled_for_removal = remove;
}


b2Body *Projectile::getBody() const
{
   return _body;
}


void Projectile::setBody(b2Body *body)
{
    _body = body;
}


void Projectile::clear()
{
    _hit_positions.clear();
    _projectiles.clear();
}


void Projectile::cleanup()
{
   _hit_positions.clear();
   for (auto it = _projectiles.begin(); it != _projectiles.end(); )
   {
      auto projectile = *it;
      if (projectile->isScheduledForRemoval())
      {
         _hit_positions.push_back(b2Vec2(projectile->getBody()->GetPosition()));
         delete *it;
         _projectiles.erase(it++);
      }
      else
      {
         ++it;
      }
   }
}


void Projectile::updateHitAnimations(const sf::Time& dt)
{
   cleanup();

   auto hitPositions = _hit_positions;

   std::list<b2Vec2>::iterator it;
   for (it = hitPositions.begin(); it != hitPositions.end(); ++it)
   {
      b2Vec2 vec = *it;
      float gx = vec.x * PPM;
      float gy = vec.y * PPM;

      ProjectileHitAnimation::add(gx, gy);
   }

   ProjectileHitAnimation::updateAnimations(dt);
}


void Projectile::setDestroyedCallback(const DestroyedCallback& destroyed_callback)
{
   _destroyed_callback = destroyed_callback;
}


bool Projectile::isSticky() const
{
   return _sticky;
}


void Projectile::setSticky(bool sticky)
{
   _sticky = sticky;
}

bool Projectile::hitSomething() const
{
   return _hit_something;
}

void Projectile::setHitSomething(bool hit_something)
{
   _hit_something = hit_something;
}

