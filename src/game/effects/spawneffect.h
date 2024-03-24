#ifndef SPAWNEFFECT_H
#define SPAWNEFFECT_H

#include <SFML/Graphics.hpp>

#include "game/animation.h"

class SpawnEffect
{
public:
   SpawnEffect(const sf::Vector2f pos_px);

   void draw(sf::RenderTarget& target);
   void update(const sf::Time& dt);
   bool isFinished() const;

private:

   struct Particle
   {
      sf::Vector2f _pos_norm;
      sf::Vector2f _pos_px;
      sf::Time _elapsed;
      sf::Time _delay;
      float _velocity{0.0f};
      float _radius_px{60.0f};
      float _scale_px{0.0f};
      sf::Vector2f _offset_px;

      float _alpha_all_particles{1.0f};
      float _alpha_per_particle{1.0f};

      sf::Sprite _sprite;

      void spawn();
      void update(const sf::Time& dt);
      void setupPosition(float random_scale);
   };

   struct ParticleEffect
   {
      ParticleEffect(const sf::Vector2f& offset_px, int32_t count, float radius_px);

      void draw(sf::RenderTarget& target);
      void update(const sf::Time& dt);

      std::vector<Particle> _particles;
      std::shared_ptr<sf::Texture> _texture;
   };

   struct Orb
   {
      Orb(const sf::Vector2f& pos_px);

      void draw(sf::RenderTarget& target);
      void update(const sf::Time& dt);

      std::shared_ptr<sf::Texture> _texture;
      std::shared_ptr<Animation> _animation_show;
      std::shared_ptr<Animation> _animation_idle;
      std::shared_ptr<Animation> _animation_hide;
      int32_t _idle_cycles{0};
   };

   std::unique_ptr<Orb> _orb;
   std::unique_ptr<ParticleEffect> _particles;
};

#endif  // SPAWNEFFECT_H
