#pragma once

#include "game/mechanisms/weather.h"

struct TmxObject;

/// \brief weather overlay that flashes screen brightness during thunderstorm phases.
class ThunderstormOverlay : public WeatherOverlay
{
public:
   /// \brief timings for lightning and silence phases in seconds.
   struct ThunderstormSettings
   {
      float _thunderstorm_time_s = 3.0;
      float _silence_time_s = 5.0f;
   };

   /// \brief creates a thunderstorm overlay with default phase timings.
   ThunderstormOverlay() = default;

   /// \brief draws a fullscreen grayscale quad with intensity derived from fbm noise.
   /// \param target SFML render target used for weather output.
   /// \param normal unused normal-map target required by the weather overlay interface.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief advances lightning/silence state timers and flash intensity factor.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;

   /// \brief sets the screen-space area covered by the lightning quad.
   /// \param newRect target rectangle in pixels.
   void setRect(const sf::FloatRect& newRect);

   /// \brief updates thunderstorm timing settings.
   /// \param newSettings durations for lightning and silence phases.
   void setSettings(const ThunderstormSettings& newSettings);

private:
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
