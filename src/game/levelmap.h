#pragma once

#include "bitmapfont.h"
#include "image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <filesystem>
#include <memory>
#include <vector>

class Door;
class Portal;

class LevelMap
{
   public:

      LevelMap();

      void loadLevelTexture(const std::filesystem::path& path);
      void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

      void setDoors(const std::vector<Door*>& doors);
      void setPortals(const std::vector<Portal*>& portals);


   private:

      void drawLevelItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

      BitmapFont mFont;
      std::map<std::string, std::shared_ptr<Layer>> mLayers;

      sf::RenderTexture mLevelRenderTexture;
      sf::Texture mLevelTexture;
      sf::Sprite mLevelSprite;

      std::vector<Door*> mDoors;
      std::vector<Portal*> mPortals;

      bool mZoomEnabled = false;
      bool mPanEnabled = false;

      float mZoom = 1.0f;
};

