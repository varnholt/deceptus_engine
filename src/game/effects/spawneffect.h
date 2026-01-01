#ifndef SPAWNEFFECT_H
#define SPAWNEFFECT_H

#include <SFML/Graphics.hpp>

#include "game/animation/animation.h"
#include "game/io/gamedeserializedata.h"

class SpawnEffect
{
public:
   SpawnEffect(const sf::Vector2f pos_px);

   void deserialize(const GameDeserializeData& data);

   void draw(sf::RenderTarget& target);
   void update(const sf::Time& dt);
   bool isFinished() const;
   bool isShown() const;
   void activate();
   bool isActive() const;

private:
   struct Particle
   {
      Particle(const sf::Texture& texture);

      sf::Vector2f _pos_norm;
      sf::Vector2f _pos_px;
      sf::Time _delay;
      float _velocity{0.0f};
      float _radius_px{0.0f};
      float _scale_px{0.0f};
      float _show_duration_s{0.0f};
      sf::Vector2f _offset_px;
      float _particle_velocity_min{0.0f};
      float _particle_velocity_max{0.0f};

      float _alpha_all_particles{1.0f};
      bool _respawn{true};
      bool _dead{false};

      std::unique_ptr<sf::Sprite> _sprite;

      void spawn();
      void update(const sf::Time& dt);
      void setupPosition(float random_scale);
   };

   struct ParticleEffect
   {
      ParticleEffect(
         const sf::Vector2f& offset_px,
         int32_t count,
         float radius_px,
         float show_duration_s,
         float _particle_velocity_min,
         float _particle_velocity_max
      );

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

      Orb(const sf::Vector2f& pos_px, int32_t idle_cycle_count);

      void draw(sf::RenderTarget& target);
      void update(const sf::Time& dt);

      std::shared_ptr<sf::Texture> _texture;
      std::shared_ptr<Animation> _animation_show;
      std::shared_ptr<Animation> _animation_idle;
      std::shared_ptr<Animation> _animation_hide;

      std::optional<std::chrono::high_resolution_clock::time_point> _hide_time_start;

      Step _step{Step::Show};
      int32_t _idle_cycle_count{0};
   };

   static constexpr float _default_hide_duration_s{2.0f};
   static constexpr float _default_show_duration_s{1.0f};
   static constexpr int32_t _default_particle_count{100};
   static constexpr float _default_particle_radius{150.0f};
   static constexpr float _default_particle_velocity_min{0.001f};
   static constexpr float _default_particle_velocity_max{0.004f};
   static constexpr int32_t _default_orb_idle_cycle_count{1};

   float _hide_duration_s{_default_hide_duration_s};
   float _show_duration_s{_default_show_duration_s};
   int32_t _particle_count{_default_particle_count};
   float _particle_radius{_default_particle_radius};
   float _particle_velocity_min{_default_particle_velocity_min};
   float _particle_velocity_max{_default_particle_velocity_max};
   int32_t _orb_idle_cycle_count{_default_orb_idle_cycle_count};

   sf::Time _elapsed_show;
   sf::Time _elapsed_hide;
   std::unique_ptr<Orb> _orb;
   std::unique_ptr<ParticleEffect> _particles;
   bool _activate{false};
};

#endif  // SPAWNEFFECT_H
