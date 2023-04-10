#include "projectile.h"

#include "audio.h"
#include "framework/math/sfmlmath.h"
#include "playerutils.h"
#include "projectilehitanimation.h"
#include "projectilehitaudio.h"

#include "Box2D/Box2D.h"

#include <iostream>
#include <memory>

namespace
{
std::vector<Projectile::HitInformation> _hit_information;
std::set<Projectile*> _projectiles;
}  // namespace

Projectile::Projectile() : FixtureNode(this)
{
   setClassName(typeid(Projectile).name());
   _type = ObjectTypeProjectile;
   _projectiles.insert(this);

   ProjectileHitAnimation::setupDefaultAnimation();
}

Projectile::~Projectile()
{
   for (auto& cb : _destroyed_callbacks)
   {
      cb();
   }

   if (_body)
   {
      _body->GetWorld()->DestroyBody(_body);
   }
}

bool Projectile::isScheduledForRemoval() const
{
   return _scheduled_for_removal;
}

void Projectile::setScheduledForRemoval(bool remove)
{
   _scheduled_for_removal = remove;
}

bool Projectile::isScheduledForInactivity() const
{
   return _scheduled_for_inactivity;
}

void Projectile::setScheduledForInactivity(bool scheduled_for_inactivity)
{
   _scheduled_for_inactivity = scheduled_for_inactivity;
}

b2Body* Projectile::getBody() const
{
   return _body;
}

void Projectile::setBody(b2Body* body)
{
   _body = body;
}

void Projectile::clear()
{
   _hit_information.clear();

   // delete all projectiles that have not been scheduled for removal until just now
   std::for_each(
      _projectiles.begin(),
      _projectiles.end(),
      [](auto projectile)
      {
         // there's no more need to notify the parent weapons since they're probably also already deleted;
         // also, we should not care about the box2d representation of the object, just avoid the memory
         // leak here. all box2d instances are deleted right after calling Projectile::clear().
         projectile->_destroyed_callbacks.clear();
         projectile->_body = nullptr;
         delete projectile;
      }
   );
   _projectiles.clear();
}

void Projectile::collectHitInformation()
{
   _hit_information.clear();

   for (auto it = _projectiles.begin(); it != _projectiles.end();)
   {
      auto projectile = *it;
      if (projectile->isScheduledForRemoval())
      {
         _hit_information.push_back(
            {b2Vec2(projectile->getBody()->GetPosition()),
             projectile->_rotation,
             projectile->_weapon_type,
             projectile->_projectile_identifier}
         );

         delete projectile;

         _projectiles.erase(it++);
      }
      else
      {
         ++it;
      }
   }
}

void Projectile::processHitInformation()
{
   const auto player_position = PlayerUtils::getPixelPositionFloat();

   std::vector<HitInformation>::iterator it;
   for (it = _hit_information.begin(); it != _hit_information.end(); ++it)
   {
      const auto& hit_info = *it;
      const auto& vec = hit_info._pos;
      const auto gx = vec.x * PPM;
      const auto gy = vec.y * PPM;

      // Log::Info() << "adding hit animation at: " << gx << ", " << gy << " angle: " << it->_angle;

      // at the moment the distance to the player is calculated and if the player is close enough, he'll hear the sound.
      // in the future this should be replaced by just doing the 'are you in the same chunk' check.
      const auto distance_px = SfmlMath::length(player_position - sf::Vector2f{gx, gy});

      if (distance_px < 600.0f)
      {
         // play sample
         const auto reference_samples = ProjectileHitAudio::getReferenceSamples(hit_info._projectile_animation_identifier);
         if (!reference_samples.empty())
         {
            ProjectileHitAudio::ProjectileHitSample sample = reference_samples[std::rand() % reference_samples.size()];
            Audio::getInstance().playSample({sample._sample, sample._volume});
         }
      }

      // play animation
      const auto reference_animation = ProjectileHitAnimation::getReferenceAnimation(hit_info._projectile_animation_identifier);
      ProjectileHitAnimation::playHitAnimation(gx, gy, it->_angle, reference_animation->second);
   }
}

std::string Projectile::getProjectileIdentifier() const
{
   return _projectile_identifier;
}

void Projectile::setProjectileIdentifier(const std::string& projectile_identifier)
{
   _projectile_identifier = projectile_identifier;
}

Animation& Projectile::getAnimation()
{
   return _animation;
}

void Projectile::setAnimation(const Animation& sprite)
{
   _animation = sprite;
}

float Projectile::getRotation() const
{
   return _rotation;
}

void Projectile::setRotation(float rotation)
{
   _rotation = rotation;
}

bool Projectile::isRotating() const
{
   return _rotating;
}

void Projectile::setRotating(bool rotating)
{
   _rotating = rotating;
}

void Projectile::update(const sf::Time& dt)
{
   // this is for projectiles that are just not hitting anything
   for (auto& projectile : _projectiles)
   {
      projectile->_time_alive += dt;
      if (projectile->_time_alive.asSeconds() > 30.0)
      {
         projectile->setScheduledForRemoval(true);
      }
   }

   collectHitInformation();
   processHitInformation();
   ProjectileHitAnimation::updateHitAnimations(dt);
}

void Projectile::addDestroyedCallback(const DestroyedCallback& destroyed_callback)
{
   _destroyed_callbacks.push_back(destroyed_callback);
}

bool Projectile::isSticky() const
{
   return _sticky;
}

void Projectile::setSticky(bool sticky)
{
   _sticky = sticky;
}

bool Projectile::hitSomething() const
{
   return _hit_something;
}

void Projectile::setHitSomething(bool hit_something)
{
   _hit_something = hit_something;
}
