#include "projectile.h"

#include "projectilehitanimation.h"

#include "Box2D/Box2D.h"

#include <iostream>
#include <memory>


std::map<WeaponType, ProjectileHitAnimation::FrameData> Projectile::_hit_animations;
std::list<Projectile::HitInformation> Projectile::_hit_information;
std::set<Projectile*> Projectile::_projectiles;



Projectile::Projectile()
 : FixtureNode(this)
{
   setName(typeid(Projectile).name());
   _type = ObjectTypeProjectile;
   _projectiles.insert(this);

   // have a default animation if case there are none yet
   if (_hit_animations.empty())
   {
      _hit_animations.emplace(WeaponType::Default, ProjectileHitAnimation::getDefaultAnimation());
   }
}


Projectile::~Projectile()
{
   for (auto& cb : _destroyed_callbacks)
   {
      cb();
   }

   _body->GetWorld()->DestroyBody(_body);
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


b2Body *Projectile::getBody() const
{
   return _body;
}


void Projectile::setBody(b2Body *body)
{
   _body = body;
}


void Projectile::clear()
{
   _hit_information.clear();
   _projectiles.clear();
}


void Projectile::collectHitInformation()
{
   _hit_information.clear();

   for (auto it = _projectiles.begin(); it != _projectiles.end(); )
   {
      auto projectile = *it;
      if (projectile->isScheduledForRemoval())
      {
         _hit_information.push_back({
               b2Vec2(projectile->getBody()->GetPosition()),
               projectile->_rotation,
               projectile->_weapon_type
            }
         );

         delete *it;

         _projectiles.erase(it++);
      }
      else
      {
         ++it;
      }
   }
}


void Projectile::addHitAnimations()
{
   std::list<HitInformation>::iterator it;
   for (it = _hit_information.begin(); it != _hit_information.end(); ++it)
   {
      const auto& hit_info = *it;
      const b2Vec2& vec = hit_info._pos;

      float gx = vec.x * PPM;
      float gy = vec.y * PPM;

      // std::cout << "adding hit animation at: " << gx << ", " << gy << " angle: " << it->_angle << std::endl;

      ProjectileHitAnimation::add(gx, gy, it->_angle, _hit_animations[hit_info._weapon_type]);
   }
}


const sf::Rect<int32_t>& Projectile::getTextureRect() const
{
   return _texture_rect;
}


void Projectile::setTextureRect(const sf::Rect<int32_t>& texture_rect)
{
   _texture_rect = texture_rect;
}


sf::Sprite Projectile::getSprite() const
{
   return _sprite;
}


void Projectile::setSprite(const sf::Sprite& sprite)
{
   _sprite = sprite;
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
   collectHitInformation();
   addHitAnimations();
   ProjectileHitAnimation::updateAnimations(dt);
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
