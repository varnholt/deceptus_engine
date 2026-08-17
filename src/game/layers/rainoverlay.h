#pragma once

#include "constants.h"
#include "game/audio/soundrotation.h"
#include "weatheroverlay.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

/// \brief weather overlay that simulates rain streaks and optional surface splashes.
class RainOverlay : public WeatherOverlay
{
public:
   /// \brief runtime parameters controlling rain density, collision behavior, and looped audio.
   struct RainSettings
   {
      bool _collide = true;
      int32_t _drop_count = 500;
      int32_t _fall_through_rate = 0;
      std::string _sound;          //!< looped rain sample played while the effect is active; empty disables audio
      float _sound_volume = 1.0f;  //!< per-sample volume multiplier applied to the looped rain sample
   };

   /// \brief state for one animated rain streak sprite.
   struct RainDrop
   {
      /// \brief respawns the drop at the top of the clip area with random delay.
      /// \param rect spawn rectangle around the player in screen space pixels.
      void reset(const sf::FloatRect& rect);

      sf::Vector2f _origin_px;
      sf::Vector2f _pos_px;
      sf::Vector2f _dir_px;
      float _length = 0.0f;
      float _age_s = 0.0f;
      std::unique_ptr<sf::Sprite> _sprite;
      std::vector<float> _intersections;
   };

   /// \brief splash animation state created when a drop collides with a surface.
   struct DropHit
   {
      sf::Vector2f _pos_px;
      float _age_s = 0.0f;
      std::unique_ptr<sf::Sprite> _sprite;
   };

   /// \brief one collidable world edge segment used for rain/surface intersection tests.
   struct Edge
   {
      sf::Vector2f _p1_px;
      sf::Vector2f _p2_px;
   };

   /// \brief creates rain drop sprites and loads the rain texture atlas.
   RainOverlay();

   /// \brief stops the looped rain sample so it does not outlive the overlay.
   ~RainOverlay() override;

   /// \brief draws active rain streaks and optional splash sprites.
   /// \param target SFML render target used for rain rendering.
   /// \param normal unused normal-map target required by the weather overlay interface.
   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;

   /// \brief draws active rain streaks and optional splash sprites with explicit render states.
   /// \param target SFML render target used for rain rendering.
   /// \param normal unused normal-map target required by the weather overlay interface.
   /// \param states render states carrying the level view and, under vrsfml, the rain texture.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;

   /// \brief updates drop motion, surface intersections, and splash lifetimes.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;

   /// \brief replaces rain simulation settings used by subsequent updates.
   /// \param newSettings new rain configuration values.
   void setSettings(const RainSettings& newSettings);

   /// \brief starts or stops the looped rain sample.
   /// \param audio_enabled true while the rain effect is active for the player.
   void setAudioEnabled(bool audio_enabled) override;

private:
   /// \brief rebuilds collidable rain surface segments from nearby box2d chain shapes.
   void determineRainSurfaces();

   /// \brief stops the looped rain sample if it is currently playing.
   void stopPlaying();

   bool _initialized = false;
   uint8_t _refresh_surface_counter = 0;

   sf::FloatRect _screen;
   sf::FloatRect _clip_rect;

   std::vector<RainDrop> _drops;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<Edge> _edges;

   std::vector<DropHit> _hits;
   Winding _winding = Winding::Clockwise;

   RainSettings _settings;

   bool _audio_enabled{false};  //!< tracks the last audio state so playback only changes on transitions
   SoundRotation _sound;        //!< runs the looped rain sample
};
