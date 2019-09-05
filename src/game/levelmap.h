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

      void loadLevelTextures(
         const std::filesystem::path& grid,
         const std::filesystem::path& outlines
      );

      void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

      void setDoors(const std::vector<std::shared_ptr<Door>>& doors);
      void setPortals(const std::vector<std::shared_ptr<Portal>>& portals);


   private:

      void drawLevelItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

      BitmapFont mFont;
      std::map<std::string, std::shared_ptr<Layer>> mLayers;

      sf::RenderTexture mLevelRenderTexture;

      sf::Texture mLevelGridTexture;
      sf::Sprite mLevelGridSprite;

      sf::Texture mLevelOutlineTexture;
      sf::Sprite mLevelOutlineSprite;

      std::vector<std::shared_ptr<Door>> mDoors;
      std::vector<std::shared_ptr<Portal>> mPortals;

      bool mZoomEnabled = false;

      float mZoom = 1.0f;
};

