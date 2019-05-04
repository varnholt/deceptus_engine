// base
#include "weapon.h"

// game
#include "bullet.h"
#include "bullethitanimation.h"
#include "constants.h"

#include <iostream>


sf::Rect<int32_t> Weapon::mEmptyRect;


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


void Weapon::fireNow(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
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
   bullet->setDestroyedCallback([this, bullet](){mBullets.erase(bullet);});
   bullet->setProperty("damage", 100);
   bullet->setBody(body);
   fixture->SetUserData(static_cast<void*>(bullet));

   // store bullet
   mBullets.insert(bullet);
}


void Weapon::fireInIntervals(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   if (mFireClock.getElapsedTime().asMilliseconds() > mFireInterval)
   {
      fireNow(world, pos, dir);

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
   for (auto bullet: mBullets)
   {
      mBulletSprite.setPosition(
         bullet->getBody()->GetPosition().x * PPM,
         bullet->getBody()->GetPosition().y * PPM
      );

      target.draw(mBulletSprite);
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
   //   std::cout << mTexturePath.string() << std::endl;
   //   std::cout
   //      << "x1: " << mTextureRect.left << " "
   //      << "y1: " << mTextureRect.top << " "
   //      << "width: " << mTextureRect.width << " "
   //      << "height: " << mTextureRect.height
   //      << std::endl;

   if (!mBulletTexture.loadFromFile(mTexturePath.string()))
   {
      std::cout << "Weapon::loadTextures(): couldn't load texture " << mTexturePath.string() << std::endl;
   }

   if (mTextureRect.width > 0)
   {
      mBulletSprite.setOrigin(
         static_cast<float_t>(mTextureRect.width / 2),
         static_cast<float_t>(mTextureRect.height / 2)
      );

      mBulletSprite.setTextureRect(mTextureRect);
      mBulletSprite.setTexture(mBulletTexture);
   }
   else
   {
      mBulletSprite.setOrigin(
         static_cast<float_t>(mBulletTexture.getSize().x / 2),
         static_cast<float_t>(mBulletTexture.getSize().y / 2)
      );

      mBulletSprite.setTexture(mBulletTexture, true);
   }
}


void Weapon::setTexture(
   const std::filesystem::path& path,
   const sf::Rect<int32_t>& textureRect
)
{
   bool reload = ((path != mTexturePath) || (textureRect != mTextureRect));

   if (reload)
   {
      mTextureRect = textureRect;
      mTexturePath = path;

      loadTextures();
   }
}


void Weapon::drawBulletHits(sf::RenderTarget& target)
{
   auto bulletHits = BulletHitAnimation::getAnimations();
   for (auto it = bulletHits->begin(); it != bulletHits->end(); ++it)
   {
      target.draw(*(*it));
   }
}

