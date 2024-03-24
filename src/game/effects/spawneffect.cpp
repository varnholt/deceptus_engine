#include "spawneffect.h"

#include <iostream>
#include "framework/math/sfmlmath.h"
#include "game/animationpool.h"
#include "game/constants.h"
#include "game/texturepool.h"

namespace
{

float frand(float min, float max)
{
   const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
   return min + val * (max - min);
};

}  // namespace

SpawnEffect::SpawnEffect(const sf::Vector2f pos_px)
{
   _particles = std::make_unique<ParticleEffect>(pos_px, 100, 150);
   _orb = std::make_unique<Orb>(pos_px);
}

void SpawnEffect::draw(sf::RenderTarget& target)
{
   _particles->draw(target);
}

void SpawnEffect::update(const sf::Time& dt)
{
   _particles->update(dt);
   _orb->update(dt);
}

bool SpawnEffect::isFinished() const
{
   return false;
   // return _orb->_animation_hide->_finished;
}

SpawnEffect::ParticleEffect::ParticleEffect(const sf::Vector2f& offset_px, int32_t count, float radius_px)
{
   // texture: 3000 x 300
   // row 1: 30 * 50 x 50 (show)
   // row 2: 60 * 50 x 50 (idle)
   // row 3: 12 * 100 x 100 (hide)
   _texture = TexturePool::getInstance().get("data/effects/spawn_particles.png");

   _particles.reserve(count);
   for (auto i = 0; i < count; i++)
   {
      Particle particle;
      particle._offset_px = offset_px;
      particle._radius_px = radius_px;
      particle._sprite.setTexture(*_texture);
      particle.spawn();
      particle.setupPosition(frand(0.0f, 1.0f));  // at the start spawn from everywhere

      _particles.push_back(particle);
   }
}

void SpawnEffect::ParticleEffect::draw(sf::RenderTarget& target)
{
   std::ranges::for_each(
      _particles,
      [&target](const auto& particle)
      {
         if (particle._delay.asMilliseconds() <= 50)
         {
            target.draw(particle._sprite);
         }
      }
   );
}

void SpawnEffect::ParticleEffect::update(const sf::Time& dt)
{
   std::ranges::for_each(_particles, [&dt](auto& particle) { particle.update(dt); });
}

void SpawnEffect::Particle::setupPosition(float random_scale)
{
   const auto angle = static_cast<float>(std::rand() % 360) * FACTOR_DEG_TO_RAD;

   const auto x = std::cos(angle);
   const auto y = std::sin(angle);

   _pos_norm.x = x;
   _pos_norm.y = y;

   _scale_px = random_scale * _radius_px;
}

void SpawnEffect::Particle::spawn()
{
   setupPosition(frand(0.7f, 1.0f));
   _velocity = frand(0.001f, 0.004f);
   _elapsed = sf::Time::Zero;
   _delay = sf::seconds(frand(0.0f, 3.0f));

   // each texture rect is 10x10px, 5 particles in 1 row
   _sprite.setTextureRect({(std::rand() % 5) * 10, 0, 10, 10});
}

void SpawnEffect::Particle::update(const sf::Time& dt)
{
   _delay -= dt;
   if (_delay.asMilliseconds() > 50)
   {
      return;
   }

   _pos_norm = _pos_norm * (1.0f - _velocity * dt.asMilliseconds());
   _pos_px = _pos_norm * _scale_px;
   _elapsed += dt;
   _sprite.setPosition(_pos_px + _offset_px);

   // respawn condition
   if (_pos_px.x * _pos_px.x + _pos_px.y * _pos_px.y < 0.1f)
   {
      spawn();
   }

   // alpha should increase within the first second after spawn or so
   // also alpha should have a static alpha so the overall effect can be activated and deactivated
   const auto alpha_value = std::min(SfmlMath::length(_pos_norm), 1.0f);
   const auto alpha = static_cast<uint8_t>(255 - (alpha_value * alpha_value * 255));
   _sprite.setColor({255, 255, 255, alpha});
}

SpawnEffect::Orb::Orb(const sf::Vector2f& pos_px)
{
   _texture = TexturePool::getInstance().get("data/effects/spawn_orb.png");

   AnimationPool animation_pool{"data/effects/spawn_effect_animations.json"};

   _animation_show = animation_pool.create("show", pos_px.x, pos_px.y, false, false);
   _animation_idle = animation_pool.create("idle", pos_px.x, pos_px.y, false, false);
   _animation_hide = animation_pool.create("hide", pos_px.x, pos_px.y, false, false);

   _animation_show->play();
}

void SpawnEffect::Orb::draw(sf::RenderTarget& target)
{
   if (!_animation_show->_paused)
   {
      _animation_show->draw(target);
   }
   else if (!_animation_idle->_paused)
   {
      _animation_idle->draw(target);
   }
   else if (!_animation_hide->_paused)
   {
      _animation_hide->draw(target);
   }
}

void SpawnEffect::Orb::update(const sf::Time& dt)
{
   if (!_animation_show->_paused)
   {
      _animation_show->update(dt);
   }

   if (_animation_show->_finished)
   {
      _animation_idle->play();
   }

   if (!_animation_idle->_paused)
   {
      _animation_idle->update(dt);
   }

   if (_animation_idle->_finished)
   {
      _animation_hide->play();
   }

   if (!_animation_hide->_paused)
   {
      _animation_hide->update(dt);
   }
}
