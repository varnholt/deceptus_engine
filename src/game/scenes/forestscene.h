#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <map>
#include <memory>

class ForestScene
{
public:
   ForestScene();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& time);

private:
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   sf::Font _font;
   std::unique_ptr<sf::Text> _text;
};
