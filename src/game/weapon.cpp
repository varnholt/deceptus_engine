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

sf::Rect<int32_t> Weapon::_empty_rect;


Weapon::Weapon()
{
   _shape = std::make_unique<b2CircleShape>();
   _shape->m_radius = 0.05f;
}


Weapon::Weapon(std::unique_ptr<b2Shape> shape, int32_t fireInterval, int32_t damage)
 : _shape(std::move(shape)),
   _fire_interval_ms(fireInterval),
   _damage(damage)
{
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
   fixtureDef.shape = _shape.get();
   fixtureDef.density = 0.0f;

   fixtureDef.filter.groupIndex   = groupIndex;
   fixtureDef.filter.maskBits     = maskBitsStanding;
   fixtureDef.filter.categoryBits = categoryBits;

   auto fixture = body->CreateFixture(&fixtureDef);

   body->ApplyLinearImpulse(
      dir,
      pos,
      true
   );

   auto projectile = new Projectile();
   projectile->addDestroyedCallback([this, projectile](){_projectiles.erase(projectile);});
   projectile->setProperty("damage", _damage);
   projectile->setBody(body);
   fixture->SetUserData(static_cast<void*>(projectile));

   // store projectile
   _projectiles.insert(projectile);
}


void Weapon::fireInIntervals(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   if (_fire_clock.getElapsedTime().asMilliseconds() > _fire_interval_ms)
   {
      fireNow(world, pos, dir);

      _fire_clock.restart();
   }
}


int Weapon::getFireIntervalMs() const
{
   return _fire_interval_ms;
}


void Weapon::setFireIntervalMs(int fireInterval)
{
   _fire_interval_ms = fireInterval;
}


void Weapon::drawProjectiles(sf::RenderTarget& target)
{
   for (auto projectile : _projectiles)
   {
      if (projectile->isScheduledForRemoval())
      {
         continue;
      }

      _projectile_sprite.setPosition(
         projectile->getBody()->GetPosition().x * PPM,
         projectile->getBody()->GetPosition().y * PPM
      );

      target.draw(_projectile_sprite);
   }
}


void Weapon::draw(sf::RenderTarget& target)
{
   drawProjectiles(target);
}


void Weapon::update(const sf::Time& /*time*/)
{
}


int Weapon::damage() const
{
   int val = 0;

   switch (_type)
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


void Weapon::initialize()
{
   loadTextures();
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

   if (!_projectile_texture.loadFromFile(_texture_path.string()))
   {
      std::cout << "Weapon::loadTextures(): couldn't load texture " << _texture_path.string() << std::endl;
   }

   if (_shape->GetType() == b2Shape::e_polygon)
   {
      _projectile_sprite.setOrigin(0, 0);
      _projectile_sprite.setTextureRect(_texture_rect);
      _projectile_sprite.setTexture(_projectile_texture);
   }
   else if (_shape->GetType() == b2Shape::e_circle)
   {
      if (_texture_rect.width > 0)
      {
         _projectile_sprite.setOrigin(
            static_cast<float_t>(_texture_rect.width / 2),
            static_cast<float_t>(_texture_rect.height / 2)
         );

         _projectile_sprite.setTextureRect(_texture_rect);
         _projectile_sprite.setTexture(_projectile_texture);
      }
      else
      {
         _projectile_sprite.setOrigin(
            static_cast<float_t>(_projectile_texture.getSize().x / 2),
            static_cast<float_t>(_projectile_texture.getSize().y / 2)
         );

         _projectile_sprite.setTexture(_projectile_texture, true);
      }
   }
}


void Weapon::setTexture(
   const std::filesystem::path& path,
   const sf::Rect<int32_t>& textureRect
)
{
   bool reload = ((path != _texture_path) || (textureRect != _texture_rect));

   if (reload)
   {
      _texture_rect = textureRect;
      _texture_path = path;

      loadTextures();
   }
}


void Weapon::drawProjectileHitAnimations(sf::RenderTarget& target)
{
   // draw projectile hits
   auto hits = ProjectileHitAnimation::getAnimations();
   for (auto it = hits->begin(); it != hits->end(); ++it)
   {
      target.draw(*(*it));
   }
}

