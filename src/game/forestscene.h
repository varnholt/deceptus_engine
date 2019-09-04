#pragma once


#include "image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>


class ForestScene
{
   public:
      ForestScene();

      void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
      void update(const sf::Time& time);

   private:
      std::vector<std::shared_ptr<Layer>> mLayerStack;
      std::map<std::string, std::shared_ptr<Layer>> mLayers;

      sf::Font mFont;
      sf::Text mText;
};

