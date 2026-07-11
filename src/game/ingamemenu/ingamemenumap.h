#pragma once

#include "framework/image/layer.h"
#include "game/image/layerdata.h"
#include "game/ingamemenu/ingamemenupage.h"
#include "layers/bitmapfont.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <filesystem>
#include <memory>
#include <vector>

class GameMechanism;

/// \brief renders the map submenu, including level overlays and world marker primitives.
class IngameMenuMap : public InGameMenuPage
{
public:
   /// \brief loads map menu layers, initializes panel groups, and reads animation timings.
   IngameMenuMap();

   /// \brief loads level grid and outline textures and prepares the intermediate render texture.
   /// \param grid path to the base grid texture image.
   /// \param outlines path to the level outline texture image.
   void loadLevelTextures(const std::filesystem::path& grid, const std::filesystem::path& outlines);

   /// \brief builds map view composition and draws the map page layers.
   /// \param window render target that receives map page rendering.
   /// \param states render states used for drawing.
#ifdef __EMSCRIPTEN__
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates{}) override;
#else
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
#endif

   /// \brief advances panorama camera state and active map page animations.
   /// \param dt elapsed frame time, currently unused by this page.
   void update(const sf::Time& dt) override;

   /// \brief stores door mechanisms that are rendered as map markers.
   /// \param doors door mechanism list for map overlay rendering.
   void setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors);

   /// \brief stores portal mechanisms that are rendered as map markers.
   /// \param portals portal mechanism list for map overlay rendering.
   void setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals);

private:
   /// \brief draws helper grid lines, doors, portals, and player marker onto the map target.
   /// \param window render target used as the map overlay canvas.
   /// \param states render states used for drawing primitives.
#ifdef __EMSCRIPTEN__
   void drawLevelItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates{});
#else
   void drawLevelItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
#endif

   /// \brief applies static map button prompt and zoom indicator layer visibility.
   void updateButtons();

   /// \brief animates panel offsets and alpha during map show and hide transitions.
   void updateShowHide();

   /// \brief animates horizontal submenu slide transitions for map panel groups.
   void updateMove();

   BitmapFont _font;

   std::unique_ptr<sf::RenderTexture> _level_render_texture;

   std::shared_ptr<sf::Texture> _level_grid_texture;
   std::unique_ptr<sf::Sprite> _level_grid_sprite;

   std::shared_ptr<sf::Texture> _level_outline_texture;
   std::unique_ptr<sf::Sprite> _level_outline_sprite;

   std::vector<std::shared_ptr<GameMechanism>> _doors;
   std::vector<std::shared_ptr<GameMechanism>> _portals;

   std::vector<LayerData> _panel_header;
   std::vector<LayerData> _panel_left;
   std::vector<LayerData> _panel_center;
   std::vector<LayerData> _panel_right;
   std::vector<LayerData> _panel_background;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;

   float _zoom = 1.0f;
};
