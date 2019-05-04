#include "bullet.h"

#include "bullethitanimation.h"

#include "Box2D/Box2D.h"

#include <memory>


std::list<b2Vec2> Bullet::sDetonationPositions;
std::set<Bullet*> Bullet::sBullets;



Bullet::Bullet()
 : FixtureNode(this),
   mScheduledForRemoval(false),
   mBody(nullptr)
{
   mType = ObjectTypeBullet;
   sBullets.insert(this);
}


Bullet::~Bullet()
{
   mDestroyedCallback();
   mBody->GetWorld()->DestroyBody(mBody);
}


Bullet::BulletType Bullet::getBulletType() const
{
   return mBulletType;
}


void Bullet::setBulletType(const BulletType &bulletType)
{
   mBulletType = bulletType;
}


bool Bullet::isScheduledForRemoval() const
{
   return mScheduledForRemoval;
}


void Bullet::setScheduledForRemoval(bool remove)
{
   mScheduledForRemoval = remove;
}


b2Body *Bullet::getBody() const
{
   return mBody;
}


void Bullet::setBody(b2Body *body)
{
   mBody = body;
}


void Bullet::cleanup()
{
   // todo: port to remove_if
   sDetonationPositions.clear();
   for (auto it = sBullets.begin(); it != sBullets.end(); )
   {
      auto bullet = *it;
      if (bullet->isScheduledForRemoval())
      {
         sDetonationPositions.push_back(b2Vec2(bullet->getBody()->GetPosition()));
         delete *it;
         sBullets.erase(it++);
      }
      else
      {
         ++it;
      }
   }
}


void Bullet::updateHitAnimations(float dt)
{
   cleanup();

   auto bulletDetonations = sDetonationPositions;

   std::list<b2Vec2>::iterator it;
   for (it = bulletDetonations.begin(); it != bulletDetonations.end(); ++it)
   {
      b2Vec2 vec = *it;
      float gx = vec.x * PPM;
      float gy = vec.y * PPM;

      BulletHitAnimation::add(gx, gy);
   }

   BulletHitAnimation::updateAnimations(dt);
}


void Bullet::setDestroyedCallback(const DestroyedCallback& destroyedCallback)
{
   mDestroyedCallback = destroyedCallback;
}
