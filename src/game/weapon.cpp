// base
#include "weapon.h"

// game
#include "constants.h"
#include "projectile.h"
#include "projectilehitanimation.h"
#include "texturepool.h"

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

   // start it so the elapsed timer is exceeded on first use
   _fire_clock.restart();
}


Weapon::Weapon(std::unique_ptr<b2Shape> shape, int32_t fireInterval, int32_t damage)
 : _shape(std::move(shape)),
   _fire_interval_ms(fireInterval),
   _damage(damage)
{
   // start it so the elapsed timer is exceeded on first use
   _fire_clock.restart();
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

   _body = world->CreateBody(&bodyDef);
   _body->SetBullet(true);
   _body->SetGravityScale(0.0f);

   b2FixtureDef fixtureDef;
   fixtureDef.shape = _shape.get();
   fixtureDef.density = 0.0f;

   fixtureDef.filter.groupIndex   = groupIndex;
   fixtureDef.filter.maskBits     = maskBitsStanding;
   fixtureDef.filter.categoryBits = categoryBits;

   auto fixture = _body->CreateFixture(&fixtureDef);

   _body->ApplyLinearImpulse(
      dir,
      pos,
      true
   );

   auto projectile = new Projectile();
   projectile->setSprite(_projectile_reference_sprite);
   projectile->setTextureRect(_projectile_reference_texture_rect);
   projectile->addDestroyedCallback([this, projectile](){
      _projectiles.erase(std::remove(_projectiles.begin(), _projectiles.end(), projectile), _projectiles.end());
   });
   projectile->setProperty("damage", _damage);
   projectile->setBody(_body);
   fixture->SetUserData(static_cast<void*>(projectile));

   // store projectile
   _projectiles.push_back(projectile);
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

      auto sprite = projectile->getSprite();

      if (projectile->isRotating())
      {
         // std::cout << "setting sprite rotation to " << projectile->getRotation() << std::endl;
         sprite.setRotation(RADTODEG * projectile->getRotation());
      }

      sprite.setPosition(
         projectile->getBody()->GetPosition().x * PPM,
         projectile->getBody()->GetPosition().y * PPM
      );

      target.draw(sprite);
   }
}


void Weapon::draw(sf::RenderTarget& target)
{
   drawProjectiles(target);
}


void Weapon::update(const sf::Time& /*time*/)
{
   // can't set the bodies inactive in the postsolve step because the world is still locked
   for (auto& projectile : _projectiles)
   {
      if (projectile->isScheduledForRemoval())
      {
         continue;
      }

      if (projectile->isScheduledForInactivity())
      {
         if (projectile->getBody()->IsActive())
         {
            projectile->getBody()->SetActive(false);
         }
      }
   }
}


int Weapon::damage() const
{
   int val = 0;

   switch (_type)
   {
      case WeaponType::Default:
      case WeaponType::Bow:
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

   _projectile_reference_texture = TexturePool::getInstance().get(_texture_path);

   if (_shape->GetType() == b2Shape::e_polygon)
   {
      _projectile_reference_sprite.setOrigin(0, 0);
      _projectile_reference_sprite.setTextureRect(_projectile_reference_texture_rect);
      _projectile_reference_sprite.setTexture(*_projectile_reference_texture);
   }
   else if (_shape->GetType() == b2Shape::e_circle)
   {
      if (_projectile_reference_texture_rect.width > 0)
      {
         _projectile_reference_sprite.setOrigin(
            static_cast<float_t>(_projectile_reference_texture_rect.width / 2),
            static_cast<float_t>(_projectile_reference_texture_rect.height / 2)
         );

         _projectile_reference_sprite.setTextureRect(_projectile_reference_texture_rect);
         _projectile_reference_sprite.setTexture(*_projectile_reference_texture);
      }
      else
      {
         _projectile_reference_sprite.setOrigin(
            static_cast<float_t>(_projectile_reference_texture->getSize().x / 2),
            static_cast<float_t>(_projectile_reference_texture->getSize().y / 2)
         );

         _projectile_reference_sprite.setTexture(*_projectile_reference_texture, true);
      }
   }
}


void Weapon::setTexture(
   const std::filesystem::path& path,
   const sf::Rect<int32_t>& textureRect
)
{
   bool reload = ((path != _texture_path) || (textureRect != _projectile_reference_texture_rect));

   if (reload)
   {
      _projectile_reference_texture_rect = textureRect;
      _texture_path = path;

      loadTextures();
   }
}


void Weapon::drawProjectileHitAnimations(sf::RenderTarget& target)
{
   // draw projectile hits
   auto hits = ProjectileHitAnimation::getAnimations();
   for (auto it = hits.begin(); it != hits.end(); ++it)
   {
      target.draw(*(*it));
   }
}

