#pragma once

#include "bitmapfont.h"
#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <filesystem>
#include <memory>
#include <vector>

class GameMechanism;

class IngameMenuMap
{
public:
   IngameMenuMap();

   void loadLevelTextures(const std::filesystem::path& grid, const std::filesystem::path& outlines);

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors);
   void setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals);

private:
   void drawLevelItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void updateButtons();

   BitmapFont _font;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   sf::RenderTexture _level_render_texture;

   std::shared_ptr<sf::Texture> _level_grid_texture;
   sf::Sprite _level_grid_sprite;

   std::shared_ptr<sf::Texture> _level_outline_texture;
   sf::Sprite _level_outline_sprite;

   std::vector<std::shared_ptr<GameMechanism>> _doors;
   std::vector<std::shared_ptr<GameMechanism>> _portals;

   bool _zoom_enabled = false;
   float _zoom = 1.0f;
};

