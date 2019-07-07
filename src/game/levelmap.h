#pragma once

#include "bitmapfont.h"
#include "image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>



class LevelMap
{
   public:

      LevelMap();
      void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);


   private:

      void drawMapInfo(sf::RenderTarget& window);

      BitmapFont mFont;
      std::vector<std::shared_ptr<Layer>> mLayerStack;
      std::map<std::string, std::shared_ptr<Layer>> mLayers;
};

