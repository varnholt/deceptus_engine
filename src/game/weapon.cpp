// base
#include "weapon.h"

// game
#include "bullet.h"
#include "bullethitanimation.h"
#include "constants.h"


std::set<Bullet*> Weapon::sBullets;
std::list<b2Vec2> Weapon::sDetonationPositions;


Weapon::Weapon()
{
   mShape = std::make_unique<b2CircleShape>();
   mShape->m_radius = 0.05f;

   loadTextures();
}


Weapon::Weapon(std::unique_ptr<b2Shape> shape, int fireInterval)
 : mShape(std::move(shape)),
   mFireInterval(fireInterval)
{
   loadTextures();
}


void Weapon::fire(
   const std::shared_ptr<b2World>& world,
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
      fixture->SetUserData(static_cast<void*>(bullet));

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


void Weapon::drawBullets(sf::RenderTarget& target)
{
   for (auto bullet: sBullets)
   {
      mBulletSprite.setPosition(
         bullet->getBody()->GetPosition().x * PPM,
         bullet->getBody()->GetPosition().y * PPM
      );

      target.draw(mBulletSprite);
   }
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


int Weapon::damage() const
{
   int val = 0;

   switch (mType)
   {
      case WeaponType::Slingshot:
      case WeaponType::Pistol:
      case WeaponType::Bazooka:
      case WeaponType::Laser:
      case WeaponType::Aliengun:
         val = 20;
         break;
   }

   return val;
}


void Weapon::loadTextures()
{
   mBulletTexture.loadFromFile("data/weapons/bullet.png");
   mBulletSprite.setTexture(mBulletTexture);
   mBulletSprite.setOrigin(
      static_cast<float_t>(mBulletTexture.getSize().x / 2),
      static_cast<float_t>(mBulletTexture.getSize().y / 2)
   );
}


void Weapon::updateBulletHitAnimations(float dt)
{
   cleanupBullets();

   auto bulletDetonations = Weapon::sDetonationPositions;

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


void Weapon::drawBulletHits(sf::RenderTarget& target)
{
   auto bulletHits = BulletHitAnimation::getAnimations();
   for (auto it = bulletHits->begin(); it != bulletHits->end(); ++it)
   {
      target.draw(*(*it));
   }
}

