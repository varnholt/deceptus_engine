#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <map>
#include <memory>
#include <string>

/// \brief renders a live gamepad input overlay from psd layers.
class ControllerOverlay
{
public:
   /// \brief loads controller overlay layers from data/game/controller.psd.
   ControllerOverlay();

   /// \brief draws the controller base and highlights currently pressed inputs.
   /// \param window SFML render target used for overlay output.
   /// \param RenderStates render state overrides passed to layer draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

private:
   std::map<std::string, std::shared_ptr<Layer>> _layers;
   sf::Vector2i _texture_size;
};
