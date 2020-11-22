// base
#include "weapon.h"

// game
#include "projectile.h"
#include "projectilehitanimation.h"
#include "constants.h"

#include <iostream>

namespace
{
   uint16_t categoryBits = CategoryEnemyCollideWith;                // I am a ...
   uint16_t maskBitsStanding = CategoryBoundary | CategoryFriendly; // I collide with ...
   int16_t groupIndex = 0;                                          // 0 is default
}

sf::Rect<int32_t> Weapon::mEmptyRect;


Weapon::Weapon()
{
   mShape = std::make_unique<b2CircleShape>();
   mShape->m_radius = 0.05f;

   loadTextures();
}


Weapon::Weapon(std::unique_ptr<b2Shape> shape, int32_t fireInterval, int32_t damage)
 : mShape(std::move(shape)),
   mFireInterval(fireInterval),
   mDamage(damage)
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
   fixtureDef.density = 0.0f;

   fixtureDef.filter.groupIndex = groupIndex;
   fixtureDef.filter.maskBits = maskBitsStanding;
   fixtureDef.filter.categoryBits = categoryBits;

   auto fixture = body->CreateFixture(&fixtureDef);

   body->ApplyLinearImpulse(
      dir,
      pos,
      true
   );

   auto projectile = new Projectile();
   projectile->setDestroyedCallback([this, projectile](){mProjectiles.erase(projectile);});
   projectile->setProperty("damage", mDamage);
   projectile->setBody(body);
   fixture->SetUserData(static_cast<void*>(projectile));

   // store projectile
   mProjectiles.insert(projectile);
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


void Weapon::drawProjectiles(sf::RenderTarget& target)
{
   for (auto projectile : mProjectiles)
   {
      mProjectileSprite.setPosition(
         projectile->getBody()->GetPosition().x * PPM,
         projectile->getBody()->GetPosition().y * PPM
      );

      target.draw(mProjectileSprite);
   }
}


int Weapon::damage() const
{
   int val = 0;

   switch (mType)
   {
      case WeaponType::Bow:
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

   if (!mProjectileTexture.loadFromFile(mTexturePath.string()))
   {
      std::cout << "Weapon::loadTextures(): couldn't load texture " << mTexturePath.string() << std::endl;
   }

   if (mShape->GetType() == b2Shape::e_polygon)
   {
      mProjectileSprite.setOrigin(0, 0);
      mProjectileSprite.setTextureRect(mTextureRect);
      mProjectileSprite.setTexture(mProjectileTexture);
   }
   else if (mShape->GetType() == b2Shape::e_circle)
   {
      if (mTextureRect.width > 0)
      {
         mProjectileSprite.setOrigin(
            static_cast<float_t>(mTextureRect.width / 2),
            static_cast<float_t>(mTextureRect.height / 2)
         );

         mProjectileSprite.setTextureRect(mTextureRect);
         mProjectileSprite.setTexture(mProjectileTexture);
      }
      else
      {
         mProjectileSprite.setOrigin(
            static_cast<float_t>(mProjectileTexture.getSize().x / 2),
            static_cast<float_t>(mProjectileTexture.getSize().y / 2)
         );

         mProjectileSprite.setTexture(mProjectileTexture, true);
      }
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


void Weapon::drawProjectileHits(sf::RenderTarget& target)
{
   auto hits = ProjectileHitAnimation::getAnimations();
   for (auto it = hits->begin(); it != hits->end(); ++it)
   {
      target.draw(*(*it));
   }
}

