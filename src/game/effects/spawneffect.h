#ifndef SPAWNEFFECT_H
#define SPAWNEFFECT_H

#include <SFML/Graphics.hpp>

#include "constants.h"

class SpawnEffect
{
public:
   SpawnEffect();

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   void update(const sf::Time& dt);

private:
   sf::Time _elapsed;

   struct Particle
   {
      sf::Vector2f _pos_px;
      sf::Time _elapsed;
      float _velocity{0.0f};
      float _radius_px{60.0f};
      sf::Vector2f _offset_px;

      float _alpha_all_particles{1.0f};
      float _alpha_per_particle{1.0f};

      sf::Sprite _sprite;

      void spawn();
      void update(const sf::Time& dt);
   };

   void createParticles(const sf::Vector2f& offset_px, int32_t count, float radius_px, const std::shared_ptr<sf::Texture>& texture);
   void drawParticles(sf::RenderTarget& target);
   void updateParticles(const sf::Time& dt);

   std::vector<Particle> _particles;
   std::shared_ptr<sf::Texture> _texture;
};

#endif  // SPAWNEFFECT_H
