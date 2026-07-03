#ifndef SPAWNEFFECT_H
#define SPAWNEFFECT_H

#include <SFML/Graphics.hpp>

#include "game/animation/animation.h"
#include "game/io/gamedeserializedata.h"

/// \brief renders an orb-and-particles spawn animation that appears, idles, and fades out.
class SpawnEffect
{
public:
   /// \brief creates the spawn effect at the given world position in pixels.
   /// \param pos_px center position used for both orb animations and particle offsets.
   SpawnEffect(const sf::Vector2f pos_px);

   /// \brief overrides default timings and particle settings from tmx object properties.
   /// \param data deserialized object data containing optional spawn effect properties.
   void deserialize(const GameDeserializeData& data);

   /// \brief draws active particles and orb animations while the effect is not finished.
   /// \param target render target.
   /// \param states render states to apply (carries .view for WASM camera transform).
   void draw(sf::RenderTarget& target, const sf::RenderStates& states = sf::RenderStates{});

   /// \brief advances particle simulation, orb animation state, and show or hide alpha ramps.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief reports whether hide animation finished and particle alpha faded out.
   /// \return true when the full spawn effect sequence is complete.
   bool isFinished() const;

   /// \brief reports whether the orb show animation has completed.
   /// \return true when the effect reached the visible idle phase.
   bool isShown() const;

   /// \brief marks the effect as activated for external game logic.
   void activate();

   /// \brief reports whether activate() was called.
   /// \return true when the effect has been marked active.
   bool isActive() const;

private:
   /// \brief one additive sprite particle that moves from a random radius toward the center.
   struct Particle
   {
      /// \brief creates a particle sprite using the shared particle texture atlas.
      /// \param texture texture used to initialize the particle sprite.
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

      /// \brief resets direction, velocity, delay, and sprite frame for a new particle cycle.
      void spawn();

      /// \brief advances delay, alpha, position, and optional respawn behavior for one frame.
      /// \param dt elapsed frame time since the previous update.
      void update(const sf::Time& dt);

      /// \brief picks a random direction and radial scale for this particle.
      /// \param random_scale normalized radius multiplier used to place the particle from center.
      void setupPosition(float random_scale);
   };

   /// \brief owns and advances a batch of particles used by the spawn effect.
   struct ParticleEffect
   {
      /// \brief creates a particle cloud with shared texture and per-particle random initialization.
      /// \param offset_px world-space pixel center where particles are rendered.
      /// \param count number of particle instances to allocate.
      /// \param radius_px maximum spawn radius in pixels.
      /// \param show_duration_s maximum randomized startup delay in seconds.
      /// \param _particle_velocity_min minimum per-particle inward velocity factor.
      /// \param _particle_velocity_max maximum per-particle inward velocity factor.
      ParticleEffect(
         const sf::Vector2f& offset_px,
         int32_t count,
         float radius_px,
         float show_duration_s,
         float _particle_velocity_min,
         float _particle_velocity_max
      );

      /// \brief draws non-delayed, non-dead particles using additive blending.
      /// \param target render target.
      /// \param states render states to apply (carries .view for WASM camera transform).
      void draw(sf::RenderTarget& target, const sf::RenderStates& states = sf::RenderStates{});

      /// \brief forwards alpha and respawn settings to particles and advances all particles.
      /// \param dt elapsed frame time since the previous update.
      void update(const sf::Time& dt);

      /// \brief reports whether every particle reached the dead state.
      /// \return true when no particle remains alive.
      bool allDead() const;

      std::vector<Particle> _particles;
      std::shared_ptr<sf::Texture> _texture;
      float _alpha{1.0f};
      bool _respawn{true};
   };

   /// \brief controls the layered orb animations that drive the spawn effect timeline.
   struct Orb
   {
      // just used for clarity
      /// \brief identifies the current phase of the orb animation state machine.
      enum class Step
      {
         Show,
         Idle,
         ParticlesDisappear,
         Hide
      };

      /// \brief creates show, idle, and hide animations and starts the show phase.
      /// \param pos_px position where all orb animations are placed.
      /// \param idle_cycle_count number of idle loops before transitioning to hide.
      Orb(const sf::Vector2f& pos_px, int32_t idle_cycle_count);

      /// \brief draws whichever orb animations are currently playing.
      /// \param target render target.
      /// \param states render states to apply (carries .view for WASM camera transform).
      void draw(sf::RenderTarget& target, const sf::RenderStates& states = sf::RenderStates{});

      /// \brief advances the orb phase machine and active animations.
      /// \param dt elapsed frame time since the previous update.
      void update(const sf::Time& dt);

      std::shared_ptr<sf::Texture> _texture;
      std::shared_ptr<Animation> _animation_show;
      std::shared_ptr<Animation> _animation_idle;
      std::shared_ptr<Animation> _animation_hide;

      float _hide_duration_s{_default_hide_duration_s};
      sf::Time _elapsed_hide;

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
