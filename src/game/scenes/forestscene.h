#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <map>
#include <memory>

/// \brief draws the post-game forest scene with layered psd artwork and moving fog.
class ForestScene
{
public:
   /// \brief loads scene layers from data/scenes/forest.psd and initializes caption text.
   ForestScene();

   /// \brief draws all scene layers and centers the message text.
   /// \param window SFML render target used for scene rendering.
   /// \param RenderStates render state overrides for layer and text draws.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   /// \brief scrolls mist layers at different speeds to create parallax motion.
   /// \param time elapsed frame time since the previous update.
   void update(const sf::Time& time);

private:
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   sf::Font _font;
   std::unique_ptr<sf::Text> _text;
};
