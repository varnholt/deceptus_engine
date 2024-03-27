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
   bool isShown() const;

private:

   struct Particle
   {
      sf::Vector2f _pos_norm;
      sf::Vector2f _pos_px;
      sf::Time _delay;
      float _velocity{0.0f};
      float _radius_px{60.0f};
      float _scale_px{0.0f};
      sf::Vector2f _offset_px;

      float _alpha_all_particles{1.0f};
      bool _respawn{true};
      bool _dead{false};

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
      bool allDead() const;

      std::vector<Particle> _particles;
      std::shared_ptr<sf::Texture> _texture;
      float _alpha{1.0f};
      bool _respawn{true};
   };

   struct Orb
   {
      // just used for clarity
      enum class Step
      {
         Show,
         Idle,
         Hide
      };

      Orb(const sf::Vector2f& pos_px);

      void draw(sf::RenderTarget& target);
      void update(const sf::Time& dt);

      std::shared_ptr<sf::Texture> _texture;
      std::shared_ptr<Animation> _animation_show;
      std::shared_ptr<Animation> _animation_idle;
      std::shared_ptr<Animation> _animation_hide;

      Step _step{Step::Show};
   };

   sf::Time _elapsed_show;
   sf::Time _elapsed_hide;
   std::unique_ptr<Orb> _orb;
   std::unique_ptr<ParticleEffect> _particles;
};

#endif  // SPAWNEFFECT_H
