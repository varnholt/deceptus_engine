#include "projectile.h"

#include "projectilehitanimation.h"

#include "Box2D/Box2D.h"

#include <memory>


std::list<b2Vec2> Projectile::sHitPositions;
std::set<Projectile*> Projectile::sProjectiles;



Projectile::Projectile()
 : FixtureNode(this)
{
   setName(typeid(Projectile).name());
   mType = ObjectTypeProjectile;
   sProjectiles.insert(this);
}


Projectile::~Projectile()
{
   mDestroyedCallback();
   mBody->GetWorld()->DestroyBody(mBody);
}


bool Projectile::isScheduledForRemoval() const
{
   return mScheduledForRemoval;
}


void Projectile::setScheduledForRemoval(bool remove)
{
   mScheduledForRemoval = remove;
}


b2Body *Projectile::getBody() const
{
   return mBody;
}


void Projectile::setBody(b2Body *body)
{
    mBody = body;
}


void Projectile::clear()
{
    sHitPositions.clear();
    sProjectiles.clear();
}


void Projectile::cleanup()
{
   sHitPositions.clear();
   for (auto it = sProjectiles.begin(); it != sProjectiles.end(); )
   {
      auto projectile = *it;
      if (projectile->isScheduledForRemoval())
      {
         sHitPositions.push_back(b2Vec2(projectile->getBody()->GetPosition()));
         delete *it;
         sProjectiles.erase(it++);
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

   auto hitPositions = sHitPositions;

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


void Projectile::setDestroyedCallback(const DestroyedCallback& destroyedCallback)
{
   mDestroyedCallback = destroyedCallback;
}

