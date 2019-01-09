// base
#include "weapon.h"

// game
#include "bullet.h"
#include "constants.h"


std::set<Bullet*> Weapon::sBullets;
std::list<b2Vec2> Weapon::sDetonationPositions;


Weapon::Weapon()
{
   mShape = std::make_unique<b2CircleShape>();
   mShape->m_radius = 0.05f;
}


Weapon::Weapon(std::unique_ptr<b2Shape> shape, int fireInterval)
 : mShape(std::move(shape)),
   mFireInterval(fireInterval)
{
}


void Weapon::fire(
   b2World *world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   if (mFireClock.getElapsedTime().asMilliseconds() > mFireInterval)
   {
      b2BodyDef bodyDef;
      bodyDef.type = b2_dynamicBody;
      bodyDef.position.Set(pos.x, pos.y);

      auto body = world->CreateBody(&bodyDef);
      body->SetBullet(true);
      body->SetGravityScale(0.0f);

      b2FixtureDef fixtureDef;
      fixtureDef.shape = mShape.get();
      fixtureDef.density = 1.0f;

      auto fixture = body->CreateFixture(&fixtureDef);

      body->ApplyLinearImpulse(
         dir,
         pos,
         true
      );

      auto bullet = new Bullet();
      bullet->setBody(body);
      fixture->SetUserData((void*)bullet);

      // store bullet
      sBullets.insert(bullet);

      mFireClock.restart();
   }
}


int Weapon::getFireInterval() const
{
   return mFireInterval;
}


void Weapon::setFireInterval(int fireInterval)
{
   mFireInterval = fireInterval;
}


void Weapon::cleanupBullets()
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





