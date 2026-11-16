#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>

/// \brief owns a configurable layered animation and its shared textures.
/// \details independent instances can be loaded and replayed; no process-wide state is retained.
class SplashAnimation
{
public:
   /// \brief loads a canvas, sprite layers and animation curves through the asset source.
   /// \param config_path path to the animation JSON in loose or packed assets.
   /// \throws std::exception when configuration or an image is invalid or unavailable.
   explicit SplashAnimation(const std::string& config_path);
   ~SplashAnimation();

   [[nodiscard]] float getDuration() const;

   /// \brief clears and renders a frame at an absolute animation time, preserving the target view.
   /// \param target borrowed render target; its dimensions determine letterboxing.
   /// \param time finite seconds since playback began; transitions clamp at either end.
   void drawFrame(sf::RenderTarget& target, float time) const;

   /// \brief plays the animation using the borrowed window's existing display settings.
   /// \param window window to render into; events are drained and desktop close requests are honored.
   /// \details returns after playback or a close request; browser builds yield between frames.
   void play(sf::RenderWindow& window) const;

private:
   struct Implementation;
   std::unique_ptr<Implementation> _implementation;
};
