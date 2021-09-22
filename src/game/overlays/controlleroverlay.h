#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <map>
#include <memory>
#include <string>

class ControllerOverlay
{
   public:

      ControllerOverlay();
      void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);


   private:

      std::map<std::string, std::shared_ptr<Layer>> _layers;
      sf::Vector2i _texture_size;

};

