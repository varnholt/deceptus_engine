#pragma once

#include "game/mechanisms/weather.h"

#include <optional>
#include <string>
#include <vector>

struct TmxObject;

/// \brief weather overlay that flashes screen brightness during thunderstorm phases.
class ThunderstormOverlay : public WeatherOverlay
{
public:
   /// \brief timings for lightning and silence phases in seconds, plus the thunder samples to pick from.
   struct ThunderstormSettings
   {
      float _thunderstorm_time_s = 3.0;
      float _silence_time_s = 5.0f;
      std::vector<std::string> _sounds;  //!< thunder samples; one is picked at random per lightning phase, empty disables audio
      float _sound_volume = 1.0f;        //!< per-sample volume multiplier applied to the picked thunder sample
   };

   /// \brief creates a thunderstorm overlay with default phase timings.
   ThunderstormOverlay() = default;

   /// \brief draws a fullscreen grayscale quad with intensity derived from fbm noise.
   /// \param target SFML render target used for weather output.
   /// \param normal unused normal-map target required by the weather overlay interface.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws the lightning quad with explicit render states.
   /// \param target SFML render target used for weather output.
   /// \param normal unused normal-map target required by the weather overlay interface.
   /// \param states render states carrying the level view.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;

   /// \brief advances lightning/silence state timers and flash intensity factor.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;

   /// \brief sets the screen-space area covered by the lightning quad.
   /// \param newRect target rectangle in pixels.
   void setRect(const sf::FloatRect& newRect);

   /// \brief updates thunderstorm timing settings.
   /// \param newSettings durations for lightning and silence phases.
   void setSettings(const ThunderstormSettings& newSettings);

   /// \brief fires one lightning phase right now instead of waiting the silence phase out.
   ///
   /// The flash is snapped to full rather than ramped in over the fifth of a second the ambient
   /// strikes take, which is what makes a scripted strike read as a crack rather than a swell.
   /// \param volume volume to play the thunder sample at, or std::nullopt for the configured one.
   void strike(const std::optional<float>& volume);

private:
   /// \brief plays one randomly picked thunder sample, if any are configured.
   /// \param volume volume to play at, or std::nullopt for the configured sound volume.
   void playThunder(const std::optional<float>& volume);

   enum class State
   {
      Lightning,
      Silence
   };

   sf::FloatRect _rect;
   float _value = 0.0f;
   float _factor = 0.0f;
   float _time_s = 0.0f;
   float _thunderstorm_time_elapsed_s = 0.0;
   float _silence_time_elapsed_s = 0.0f;
   State _state = State::Silence;

   ThunderstormSettings _settings;
};
