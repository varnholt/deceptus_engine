#ifndef BULLET_H
#define BULLET_H

// base
#include "gamenode.h"
#include "fixturenode.h"

class b2Body;

class Bullet : public FixtureNode
{

public:

   enum BulletType
   {
      BulletTypePistol
   };


protected:

   BulletType mBulletType;
   bool mScheduledForRemoval;
   b2Body* mBody;


public:

   Bullet();
   ~Bullet();

   BulletType getBulletType() const;
   void setBulletType(const BulletType &type);

   bool isScheduledForRemoval() const;
   void setScheduledForRemoval(bool isScheduledForRemoval);

   b2Body *getBody() const;
   void setBody(b2Body *body);
};


#endif // BULLET_H
