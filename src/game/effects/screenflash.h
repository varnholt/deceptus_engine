#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <memory>

/// \brief draws a short full-screen colour flash over the level.
/// \details deliberately not a ScreenTransitionEffect: those set Display::ScreenTransition, which
///          forces the player into an idle animation for as long as they run.
class ScreenFlash
{
public:
   /// \brief returns the global screen flash instance.
   static ScreenFlash& getInstance();

   /// \brief starts a flash, replacing whatever was running.
   /// \param color flash color.
   /// \param peak_intensity opacity the flash starts at, 0-1.
   /// \param duration_s time the flash needs to fade out completely.
   void flash(const sf::Color& color, float peak_intensity, float duration_s);

   /// \brief advances the fade-out.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   /// \brief draws the full-screen quad if a flash is running.
   /// \param target render texture that receives the overlay.
   void draw(const std::shared_ptr<sf::RenderTexture>& target);

private:
   ScreenFlash() = default;

   sf::Color _color{sf::Color::White};  //!< color the screen is flashed with
   float _peak_intensity{0.0f};         //!< opacity at the start of the flash, 0-1
   float _duration_s{0.0f};             //!< total fade-out duration; 0 means no flash is running
   float _elapsed_s{0.0f};              //!< time since the flash was triggered
   float _intensity{0.0f};              //!< current opacity, 0-1
};
