// base
#include "game/weapons/gun.h"

// game
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/weapons/projectile.h"
#include "game/weapons/projectilehitanimation.h"

#include <iostream>

namespace
{
uint16_t category_bits = CategoryEnemyCollideWith;                  // I am a ...
uint16_t mask_bits_standing = CategoryBoundary | CategoryFriendly;  // I collide with ...
int16_t group_index = 0;                                            // 0 is default
}  // namespace

sf::Rect<int32_t> Gun::_empty_rect;

Gun::Gun() : _shape(std::make_unique<b2CircleShape>())
{
   _type = WeaponType::Gun;
   _shape->m_radius = 0.05f;

   // start it so the elapsed timer is exceeded on first use
   _fire_clock.restart();

   // a default
   setProjectileAnimation(TexturePool::getInstance().get(_projectile_reference_animation._texture_path));
}

Gun::Gun(const WeaponProperties& properties) : _shape(std::move(properties._shape))
{
   _use_interval_ms = properties.read<int32_t>("use_interval_ms", 100);
   _damage = properties.read<int32_t>("damage", 100);
   _gravity_scale = properties.read<float>("gravity_scale", 0.0f);
   _density = properties.read<float>("density", 1.0f);

   // start it so the elapsed timer is exceeded on first use
   _fire_clock.restart();

   setProjectileAnimation(TexturePool::getInstance().get(_projectile_reference_animation._texture_path));
}

void Gun::copyReferenceAnimation(Projectile* projectile)
{
   Animation animation(_projectile_reference_animation._animation);
   animation._reset_to_first_frame = false;
   animation.updateVertices();
   animation.play();

   projectile->setAnimation(animation);
}

void Gun::use(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir)
{
   b2BodyDef body_definition;
   body_definition.type = b2_dynamicBody;
   body_definition.position.Set(pos.x, pos.y);

   auto bullet_body = world->CreateBody(&body_definition);
   bullet_body->SetBullet(true);
   bullet_body->SetGravityScale(_gravity_scale);

   b2FixtureDef fixture_definition;
   fixture_definition.shape = _shape.get();
   fixture_definition.density = _density;

   fixture_definition.filter.groupIndex = group_index;
   fixture_definition.filter.maskBits = mask_bits_standing;
   fixture_definition.filter.categoryBits = category_bits;
   auto fixture = bullet_body->CreateFixture(&fixture_definition);

   bullet_body->ApplyLinearImpulse(dir, pos, true);

   // create a projectile animation copy from the reference animation
   auto projectile = new Projectile();
   copyReferenceAnimation(projectile);

   projectile->setProperty("damage", _damage);
   projectile->setBody(bullet_body);

   projectile->addDestroyedCallback(
      [this, projectile]() { _projectiles.erase(std::remove(_projectiles.begin(), _projectiles.end(), projectile), _projectiles.end()); }
   );

   if (_projectile_reference_animation._identifier.has_value())
   {
      projectile->setProjectileIdentifier(_projectile_reference_animation._identifier.value());
   }

   fixture->SetUserData(static_cast<void*>(projectile));

   // store audio update data if present
   if (_parent_audio_update_data.has_value())
   {
      projectile->setParentAudioUpdateData(_parent_audio_update_data.value());
   }

   // store projectile
   _projectiles.push_back(projectile);
}

void Gun::useInIntervals(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir)
{
   if (_fire_clock.getElapsedTime().asMilliseconds() > _use_interval_ms)
   {
      use(world, pos, dir);

      _fire_clock.restart();
   }
}

int32_t Gun::getUseIntervalMs() const
{
   return _use_interval_ms;
}

void Gun::setUseIntervalMs(int32_t use_intervals)
{
   _use_interval_ms = use_intervals;
}

void Gun::updateProjectiles(const sf::Time& time)
{
   for (auto projectile : _projectiles)
   {
      auto& projectile_animation = projectile->getAnimation();

      if (projectile->isRotating())
      {
         projectile_animation.setRotation(FACTOR_RAD_TO_DEG * projectile->getRotation());
      }

      projectile_animation.setPosition(projectile->getBody()->GetPosition().x * PPM, projectile->getBody()->GetPosition().y * PPM);

      projectile_animation.update(time);
   }
}

void Gun::drawProjectiles(sf::RenderTarget& target)
{
   for (auto projectile : _projectiles)
   {
      target.draw(projectile->getAnimation());
   }
}

std::optional<std::string> Gun::getProjectileIdentifier() const
{
   return _projectile_reference_animation._identifier;
}

void Gun::setProjectileIdentifier(const std::string& projectile_identifier)
{
   _projectile_reference_animation._identifier = projectile_identifier;
}

void Gun::draw(sf::RenderTarget& target)
{
   drawProjectiles(target);
}

void Gun::update(const sf::Time& time)
{
   // update all projectile animations
   updateProjectiles(time);

   // can't set the bodies inactive in the postsolve step because the world is still locked
   for (const auto* projectile : _projectiles)
   {
      if (projectile->isScheduledForRemoval())
      {
         continue;
      }

      if (projectile->isScheduledForInactivity())
      {
         if (projectile->getBody()->IsEnabled())
         {
            projectile->getBody()->SetEnabled(false);
         }
      }
   }
}

int32_t Gun::getDamage() const
{
   return 20;
}

std::string Gun::getName() const
{
   return "gun";
}

// create a reference animation from a single frame
void Gun::setProjectileAnimation(const std::shared_ptr<sf::Texture>& texture, const sf::Rect<int32_t>& texture_rect_px)
{
   sf::Rect<int32_t> tmp_rect_px = texture_rect_px;
   if (tmp_rect_px.width == 0)
   {
      tmp_rect_px.width = texture->getSize().x;
      tmp_rect_px.height = texture->getSize().y;
   }

   _projectile_reference_animation._animation.setTextureRect(tmp_rect_px);
   _projectile_reference_animation._animation._color_texture = texture;
   _projectile_reference_animation._animation._frames.clear();
   _projectile_reference_animation._animation._frames.push_back(tmp_rect_px);
   _projectile_reference_animation._animation.setFrameTimes({sf::seconds(0.1f)});

   // auto-generate origin from shape
   // this should move into the luanode; the engine should not 'guess' the origin
   if (_shape->GetType() == b2Shape::e_polygon)
   {
      _projectile_reference_animation._animation.setOrigin(0, 0);
   }
   else if (_shape->GetType() == b2Shape::e_circle)
   {
      const auto origin_x_px = static_cast<float>(tmp_rect_px.width / 2);
      const auto origin_y_px = static_cast<float>(tmp_rect_px.height / 2);
      _projectile_reference_animation._animation.setOrigin(origin_x_px, origin_y_px);
   }
}

// create a reference animation from multiple frames
void Gun::setProjectileAnimation(const AnimationFrameData& frame_data)
{
   _projectile_reference_animation._animation._color_texture = frame_data._texture;
   _projectile_reference_animation._animation.setOrigin(frame_data._origin);
   _projectile_reference_animation._animation._frames = frame_data._frames;
   _projectile_reference_animation._animation.setFrameTimes(frame_data._frame_times);
}

void Gun::drawProjectileHitAnimations(sf::RenderTarget& target)
{
   // draw projectile hits
   const auto& hit_animations = ProjectileHitAnimation::getHitAnimations();
   for (auto hit_animation : hit_animations)
   {
      target.draw(*hit_animation);
   }
}
