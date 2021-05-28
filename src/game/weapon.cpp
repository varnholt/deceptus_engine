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


void Weapon::copyReferenceAnimation(Projectile* projectile)
{
   Animation animation(_projectile_reference_animation._animation);
   animation._reset_to_first_frame = false;
   animation.updateVertices();
   animation.play();

   projectile->setAnimation(animation);
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

   // create a projectile animation copy from the reference animation
   copyReferenceAnimation(projectile);

   projectile->setProperty("damage", _damage);
   projectile->setBody(_body);

   projectile->addDestroyedCallback([this, projectile](){
      _projectiles.erase(std::remove(_projectiles.begin(), _projectiles.end(), projectile), _projectiles.end());
   });

   if (_projectile_reference_animation._identifier.has_value())
   {
      projectile->setProjectileIdentifier(_projectile_reference_animation._identifier.value());
   }

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


void Weapon::updateProjectiles(const sf::Time& time)
{
   for (auto projectile : _projectiles)
   {
      auto& projectile_animation = projectile->getAnimation();

      if (projectile->isRotating())
      {
         // std::cout << "setting sprite rotation to " << projectile->getRotation() << std::endl;
         projectile_animation.setRotation(RADTODEG * projectile->getRotation());
      }

      projectile_animation.setPosition(
         projectile->getBody()->GetPosition().x * PPM,
         projectile->getBody()->GetPosition().y * PPM
      );

      projectile_animation.update(time);
   }
}


void Weapon::drawProjectiles(sf::RenderTarget& target)
{
   for (auto projectile : _projectiles)
   {
      target.draw(projectile->getAnimation());
   }
}


std::optional<std::string> Weapon::getProjectileIdentifier() const
{
   return _projectile_reference_animation._identifier;
}


void Weapon::setProjectileIdentifier(const std::string& projectile_identifier)
{
   _projectile_reference_animation._identifier = projectile_identifier;
}


void Weapon::draw(sf::RenderTarget& target)
{
   drawProjectiles(target);
}


void Weapon::update(const sf::Time& time)
{
   // update all projectile animations
   updateProjectiles(time);

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
}


// create a reference animation from a single frame
void Weapon::setProjectileAnimation(
   const std::shared_ptr<sf::Texture>& texture,
   const sf::Rect<int32_t>& textureRect
)
{
   _projectile_reference_animation._animation.setTextureRect(textureRect);
   _projectile_reference_animation._animation._color_texture = texture;
   _projectile_reference_animation._animation._frames.clear();
   _projectile_reference_animation._animation._frames.push_back(textureRect);

   // auto-generate origin from shape
   // this should move into the luanode; the engine should not 'guess' the origin
   if (_shape->GetType() == b2Shape::e_polygon)
   {
      _projectile_reference_animation._animation.setOrigin(0, 0);
   }
   else if (_shape->GetType() == b2Shape::e_circle)
   {
      if (textureRect.width > 0)
      {
         _projectile_reference_animation._animation.setOrigin(
            static_cast<float_t>(textureRect.width / 2),
            static_cast<float_t>(textureRect.height / 2)
         );
      }
      else
      {
         _projectile_reference_animation._animation.setOrigin(
            static_cast<float_t>(texture->getSize().x / 2),
            static_cast<float_t>(texture->getSize().y / 2)
         );
      }
   }
}


// create a reference animation from multiple frames
void Weapon::setProjectileAnimation(
   const AnimationFrameData& frame_data
)
{
   _projectile_reference_animation._animation._color_texture = frame_data._texture;
   _projectile_reference_animation._animation.setOrigin(frame_data._origin);
   _projectile_reference_animation._animation._frames = frame_data._frames;
   _projectile_reference_animation._animation.setFrameTimes(frame_data._frame_times);
}


void Weapon::drawProjectileHitAnimations(sf::RenderTarget& target)
{
   // draw projectile hits
   auto hits = ProjectileHitAnimation::getHitAnimations();
   for (auto it = hits.begin(); it != hits.end(); ++it)
   {
      target.draw(*(*it));
   }
}

