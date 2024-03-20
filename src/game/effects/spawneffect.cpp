#include "spawneffect.h"

#include "game/texturepool.h"

SpawnEffect::SpawnEffect()
{
   _texture = TexturePool::getInstance().get("data/effects/spawn_particles.png");
}

void SpawnEffect::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   drawParticles(target);
}

void SpawnEffect::update(const sf::Time& dt)
{
   _elapsed += dt;
   updateParticles(dt);
}

void SpawnEffect::createParticles(
   const sf::Vector2f& offset_px,
   int32_t count,
   float radius_px,
   const std::shared_ptr<sf::Texture>& texture
)
{
   _particles.reserve(count);
   for (auto i = 0; i < count; i++)
   {
      Particle particle;
      particle._offset_px = offset_px;
      particle._radius_px = radius_px;
      particle._sprite.setTexture(*texture);
      particle.spawn();
      _particles.push_back(particle);
   }
}

void SpawnEffect::drawParticles(sf::RenderTarget& target)
{
   std::ranges::for_each(_particles, [&target](const auto& particle) { target.draw(particle._sprite); });
}

void SpawnEffect::updateParticles(const sf::Time& dt)
{
   std::ranges::for_each(_particles, [&dt](auto& particle) { particle.update(dt); });
}

void SpawnEffect::Particle::spawn()
{
   auto frand = [](float min, float max)
   {
      const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
      return min + val * (max - min);
   };

   const auto random_scale = frand(0.8f, 1.2f);
   const auto angle = static_cast<float>(std::rand() % 360) * FACTOR_DEG_TO_RAD;

   const auto x = std::cos(angle);
   const auto y = std::sin(angle);

   _pos_px.x = x * random_scale * _radius_px;
   _pos_px.y = y * random_scale * _radius_px;

   _elapsed = sf::Time::Zero;

   // each texture rect is 10x10px, 5 particles in 1 row
   _sprite.setTextureRect({(std::rand() % 5) * 10, 0, 10, 10});
}

void SpawnEffect::Particle::update(const sf::Time& dt)
{
   _pos_px = _pos_px * (1.0f - _velocity * dt.asSeconds());
   _elapsed += dt;
   _sprite.setPosition(_pos_px + _offset_px);

   // respawn condition
   if (_pos_px.x * _pos_px.x + _pos_px.y * _pos_px.y < 0.1f)
   {
      spawn();
   }

   // alpha should increase within the first second after spawn or so
   // also alpha should have a static alpha so the overall effect can be activated and deactivated
}
