#include "bullet.h"

#include "Box2D/Box2D.h"
#include <memory>

Bullet::Bullet()
 : FixtureNode(this),
   mScheduledForRemoval(false),
   mBody(0)
{
   mType = ObjectTypeBullet;
}


Bullet::~Bullet()
{
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



