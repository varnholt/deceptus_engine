#pragma once

#include "framework/image/layer.h"
#include "game/image/layerdata.h"
#include "game/ingamemenu/ingamemenupage.h"
#include "layers/bitmapfont.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class GameMechanism;
class LevelMap;

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

   /// \brief resets the pan offset so the map opens centered on the player.
   void show() override;

   /// \brief cycles through the available zoom levels.
   /// \param key pressed keyboard key to interpret.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief pans the map view to the left.
   void left() override;

   /// \brief pans the map view to the right.
   void right() override;

   /// \brief pans the map view up.
   void up() override;

   /// \brief pans the map view down.
   void down() override;

   /// \brief stores door mechanisms that are rendered as map markers.
   /// \param doors door mechanism list for map overlay rendering.
   void setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors);

   /// \brief stores portal mechanisms that are rendered as map markers.
   /// \param portals portal mechanism list for map overlay rendering.
   void setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals);

private:
   /// \brief one icon type that can be placed on the map.
   struct MarkerStyle
   {
      const char* const* _rows = nullptr;  //!< 7 rows of 7 characters, '.' is transparent
      sf::Color _color;
   };

   /// \brief composes the explored level map and blits it into the map page viewport.
   /// \param window render target that receives the composed map.
   /// \param states render states used for drawing.
   void drawLevel(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws the explored parts of the level map into the map render texture.
   /// \param level_map map texture and coordinate helpers of the active level.
   void drawExploredRooms(const LevelMap& level_map);

   /// \brief draws mechanism markers and the player position on top of the revealed map.
   /// \param level_map map texture and coordinate helpers of the active level.
   void drawMarkers(const LevelMap& level_map);

   /// \brief draws one 7x7 icon centered on a map position, one icon pixel per map pixel.
   /// \param target render target receiving the icon.
   /// \param style icon pixel pattern and color.
   /// \param center_map_px icon center in map pixels.
   void drawMarker(sf::RenderTarget& target, const MarkerStyle& style, const sf::Vector2f& center_map_px);

   /// \brief recreates the map render texture when the available page area changed.
   /// \param size_px required render texture size in screen pixels.
   void updateRenderTexture(const sf::Vector2u& size_px);

   /// \brief moves the map view away from the player position and clamps it to the level bounds.
   /// \param direction pan direction, each component in range -1..1.
   void pan(const sf::Vector2f& direction);

   /// \brief selects the neighbouring detail level of the level map.
   /// \param direction -1 zooms in, 1 zooms out.
   void zoom(int32_t direction);

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

   int32_t _zoom_level = 1;     //!< selected level map detail level, mirrored by the zoom_level_* layers
   sf::Vector2f _pan_world_px;  //!< offset of the map view relative to the player position, in world pixels
   float _blink_time_s = 0.0f;
   float _alpha = 1.0f;           //!< page fade factor, applied to the map so it fades in and out with the rest
   float _move_offset_px = 0.0f;  //!< horizontal submenu slide offset, applied to the map as well
};
