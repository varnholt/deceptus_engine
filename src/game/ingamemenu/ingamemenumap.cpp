#include "ingamemenumap.h"

#include "framework/easings/easings.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/camera/camerapanorama.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/debug/console.h"
#include "game/ingamemenu/menuconfig.h"
#include "game/io/texturepool.h"
#include "game/level/levelmap.h"
#include "game/level/levelregistry.h"
#include "game/level/room.h"
#include "game/mechanisms/door.h"
#include "game/mechanisms/portal.h"
#include "game/player/playerregistry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{

//! area of the map page that shows the level, in menu pixels. everything outside is header,
//! footer and the panels that slide in from the sides.
constexpr auto map_viewport_x_px = 10.0f;
constexpr auto map_viewport_y_px = 42.0f;
constexpr auto map_viewport_width_px = 620.0f;
constexpr auto map_viewport_height_px = 290.0f;

//! how far one navigate key press moves the map view, in menu pixels
constexpr auto pan_step_px = 40.0f;

constexpr auto blink_interval_s = 1.0f;

// clang-format off
constexpr const char* icon_checkpoint[] = {
   "..xxx..",
   ".x...x.",
   "x..xx.x",
   "x.x.x.x",
   "x.xx..x",
   ".x...x.",
   "..xxx..",
};

constexpr const char* icon_portal[] = {
   "..xxx..",
   ".x...x.",
   "x..x..x",
   "x.xxx.x",
   "x..x..x",
   ".x...x.",
   "..xxx..",
};

constexpr const char* icon_door[] = {
   ".......",
   "..xxx..",
   "..x.x..",
   "..x.x..",
   "..x.x..",
   "..xxx..",
   ".......",
};

constexpr const char* icon_player[] = {
   "...x...",
   "..xxx..",
   ".xxxxx.",
   "xxxxxxx",
   ".xxxxx.",
   "..xxx..",
   "...x...",
};
// clang-format on

constexpr auto icon_size = 7;

}  // namespace

IngameMenuMap::IngameMenuMap()
{
   _font.load("data/game/font.png", "data/game/font.map");
   _filename = "data/game/map.psd";

   load();
   updateButtons();

   _panel_left = {
      _layers["cpan_bg"],
      _layers["cpan_right"],
      _layers["cpan_left"],
      _layers["cpan_down"],
      _layers["cpan_up"],
      _layers["map_keys"],
   };

   _panel_center = {
      _layers["zone_name_label_crypts"],
   };

   _panel_right = {
      _layers["zoom_level_1"],
      _layers["zoom_level_2"],
      _layers["zoom_level_3"],
      _layers["zoom_level_4"],
   };

   // clang-format off
   _panel_header = {
      _layers["close_pc_0"],
      _layers["close_pc_1"],
      _layers["close_xbox_0"],
      _layers["close_xbox_1"],
      _layers["footer_bg"],
      _layers["header"],
      _layers["header_bg"],
      _layers["next_menu_0"],
      _layers["next_menu_1"],
      _layers["previous_menu_0"],
      _layers["previous_menu_1"],
      _layers["close_xbox_0"],
      _layers["close_xbox_1"],
      _layers["close_pc_0"],
      _layers["close_pc_1"],
      _layers["legend_xbox_0"],
      _layers["legend_xbox_1"],
      _layers["legend_pc_0"],
      _layers["legend_pc_1"],
      _layers["world_xbox_0"],
      _layers["world_xbox_1"],
      _layers["world_pc_0"],
      _layers["world_pc_1"],
      _layers["zoom_xbox_0"],
      _layers["zoom_xbox_1"],
      _layers["zoom_pc_0"],
      _layers["zoom_pc_1"],
      _layers["navigate_xbox_0"],
      _layers["navigate_xbox_1"],
      _layers["navigate_pc_0"],
      _layers["navigate_pc_1"],
   };
   // clang-format on

   _panel_background = {
      _layers["bg"],
   };

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;
}

void IngameMenuMap::loadLevelTextures(const std::filesystem::path& grid, const std::filesystem::path& outlines)
{
   _level_grid_texture = TexturePool::getInstance().get(grid.string());
   _level_outline_texture = TexturePool::getInstance().get(outlines.string());

#ifdef __EMSCRIPTEN__
   _level_grid_sprite = std::make_unique<sf::Sprite>();
   _level_outline_sprite = std::make_unique<sf::Sprite>();
#else
   _level_grid_sprite = std::make_unique<sf::Sprite>(*_level_grid_texture);
   _level_outline_sprite = std::make_unique<sf::Sprite>(*_level_outline_texture);
#endif
}

void IngameMenuMap::updateRenderTexture(const sf::Vector2u& size_px)
{
   if (_level_render_texture && _level_render_texture->getSize() == size_px)
   {
      return;
   }

#ifdef __EMSCRIPTEN__
   auto created_texture = sf::RenderTexture::create(size_px);
   if (!created_texture.hasValue())
   {
      Log::Error() << "failed to create map render texture";
      return;
   }
   _level_render_texture = std::make_unique<sf::RenderTexture>(std::move(*created_texture));
#else
   try
   {
      _level_render_texture = std::make_unique<sf::RenderTexture>(size_px);
   }
   catch (const std::exception& e)
   {
      Log::Fatal() << "failed to create map render texture: " << e.what();
   }
#endif
}

void IngameMenuMap::drawExploredRooms(const LevelMap& level_map)
{
   const auto level = LevelRegistry::getCurrent();
   const auto detail_level = static_cast<size_t>(_zoom_level);

   // one quad per visited sub-room, all textured from the same level map texture. the texture is
   // opaque, so overlapping sub-rooms simply write the same pixels twice.
   sf::VertexArray quads(sf::PrimitiveType::Triangles);

   for (const auto& room : level->getRooms())
   {
      for (const auto& sub_room : room->_sub_rooms)
      {
         if (!sub_room._visited)
         {
            continue;
         }

         const auto rect = level_map.toMap(sub_room._rect, detail_level);

         const auto left = rect.position.x;
         const auto top = rect.position.y;
         const auto right = rect.position.x + rect.size.x;
         const auto bottom = rect.position.y + rect.size.y;

         const auto top_left = sf::Vector2f{left, top};
         const auto top_right = sf::Vector2f{right, top};
         const auto bottom_right = sf::Vector2f{right, bottom};
         const auto bottom_left = sf::Vector2f{left, bottom};

         // position and texture coordinate are identical because the map view works in map pixels
         quads.append(sf::Vertex{top_left, sf::Color::White, top_left});
         quads.append(sf::Vertex{top_right, sf::Color::White, top_right});
         quads.append(sf::Vertex{bottom_right, sf::Color::White, bottom_right});

         quads.append(sf::Vertex{top_left, sf::Color::White, top_left});
         quads.append(sf::Vertex{bottom_right, sf::Color::White, bottom_right});
         quads.append(sf::Vertex{bottom_left, sf::Color::White, bottom_left});
      }
   }

   sf::RenderStates states;
   states.texture = level_map.getTexture(detail_level);
   _level_render_texture->draw(quads, states);
}

void IngameMenuMap::drawMarker(sf::RenderTarget& target, const MarkerStyle& style, const sf::Vector2f& center_map_px)
{
   sf::VertexArray quads(sf::PrimitiveType::Triangles);

   // snapped to whole map pixels so the icon stays crisp
   const auto origin_x = std::round(center_map_px.x) - icon_size * 0.5f;
   const auto origin_y = std::round(center_map_px.y) - icon_size * 0.5f;

   for (auto row = 0; row < icon_size; row++)
   {
      for (auto column = 0; column < icon_size; column++)
      {
         if (style._rows[row][column] == '.')
         {
            continue;
         }

         const auto left = origin_x + static_cast<float>(column);
         const auto top = origin_y + static_cast<float>(row);
         const auto right = left + 1.0f;
         const auto bottom = top + 1.0f;

         quads.append(sf::Vertex{sf::Vector2f{left, top}, style._color});
         quads.append(sf::Vertex{sf::Vector2f{right, top}, style._color});
         quads.append(sf::Vertex{sf::Vector2f{right, bottom}, style._color});

         quads.append(sf::Vertex{sf::Vector2f{left, top}, style._color});
         quads.append(sf::Vertex{sf::Vector2f{right, bottom}, style._color});
         quads.append(sf::Vertex{sf::Vector2f{left, bottom}, style._color});
      }
   }

   target.draw(quads);
}

void IngameMenuMap::drawMarkers(const LevelMap& level_map)
{
   const auto level = LevelRegistry::getCurrent();
   const auto& rooms = level->getRooms();

   // markers are only known once the sub-room they sit in has been visited
   const auto is_discovered = [&rooms](const sf::FloatRect& bounding_box_px)
   {
      return std::any_of(
         rooms.cbegin(),
         rooms.cend(),
         [&bounding_box_px](const auto& room)
         {
            return std::any_of(
               room->_sub_rooms.cbegin(),
               room->_sub_rooms.cend(),
               [&bounding_box_px](const auto& sub_room)
               { return sub_room._visited && sfcompat::findIntersection(sub_room._rect, bounding_box_px).has_value(); }
            );
         }
      );
   };

   // the map texture is drawn without scaling, so one map pixel is one menu pixel at every zoom
   // step and the icons keep their size on screen without any correction
   const auto detail_level = static_cast<size_t>(_zoom_level);

   const auto draw_mechanisms = [this, &level_map, &is_discovered, detail_level](
                                   const std::vector<std::shared_ptr<GameMechanism>>& mechanisms, const MarkerStyle& style
                                )
   {
      for (const auto& mechanism : mechanisms)
      {
         const auto bounding_box_px = mechanism->getBoundingBoxPx();
         if (!bounding_box_px.has_value())
         {
            continue;
         }

         if (!is_discovered(bounding_box_px.value()))
         {
            continue;
         }

         const auto center_px = bounding_box_px->position + bounding_box_px->size * 0.5f;
         drawMarker(*_level_render_texture, style, level_map.toMap(center_px, detail_level));
      }
   };

   const auto& mechanism_registry = level->getMechanismRegistry();
   draw_mechanisms(mechanism_registry.getPortals(), MarkerStyle{icon_portal, sf::Color{86, 200, 255}});
   draw_mechanisms(mechanism_registry.getCheckpoints(), MarkerStyle{icon_checkpoint, sf::Color{86, 240, 128}});
   draw_mechanisms(mechanism_registry.getDoors(), MarkerStyle{icon_door, sf::Color{255, 214, 92}});

   // the player marker blinks so it stays visible against the map fill
   if (std::fmod(_blink_time_s, blink_interval_s) < blink_interval_s * 0.7f)
   {
      const auto player_position_px = PlayerRegistry::getFirst()->getPixelPositionFloat();
      drawMarker(*_level_render_texture, MarkerStyle{icon_player, sf::Color::White}, level_map.toMap(player_position_px, detail_level));
   }
}

void IngameMenuMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
#ifdef __EMSCRIPTEN__
   applyPageView(states);
#else
   const auto view_width = GameConfiguration::getInstance()._view_width;
   const auto view_height = GameConfiguration::getInstance()._view_height;
   window.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(view_width), static_cast<float>(view_height)})));
#endif

   // the map is composed between the page background and everything else, so header, footer and
   // the side panels stay on top of it
   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }

      if (layer->_name == "bg")
      {
         drawLevel(window, states);
      }
   }
}

void IngameMenuMap::drawLevel(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      return;
   }

   const auto& level_map = level->getLevelMap();
   if (!level_map.isValid())
   {
      return;
   }

   const auto viewport_size = sf::Vector2u{static_cast<uint32_t>(map_viewport_width_px), static_cast<uint32_t>(map_viewport_height_px)};
   updateRenderTexture(viewport_size);

   if (!_level_render_texture)
   {
      return;
   }

   // the view shows the selected detail level at 1:1, so one map pixel is one menu pixel and the
   // one pixel wall outlines are never scaled away. the center is snapped to whole map pixels for
   // the same reason.
   const auto detail_level = static_cast<size_t>(_zoom_level);
   const auto player_position_map_px = level_map.toMap(PlayerRegistry::getFirst()->getPixelPositionFloat(), detail_level);
   const auto pan_map_px = level_map.toMap(_pan_world_px, detail_level);
   const auto center_map_px =
      sf::Vector2f{std::round(player_position_map_px.x + pan_map_px.x), std::round(player_position_map_px.y + pan_map_px.y)};
   const auto view_size = sf::Vector2f{map_viewport_width_px, map_viewport_height_px};

#ifdef __EMSCRIPTEN__
   const sf::View map_view{.center = center_map_px, .size = view_size};
#else
   sf::View map_view;
   map_view.setSize(view_size);
   map_view.setCenter(center_map_px);
#endif

   _level_render_texture->clear(sf::Color::Transparent);
   _level_render_texture->setView(map_view);

   drawExploredRooms(level_map);
   drawMarkers(level_map);

   _level_render_texture->display();

   // the map slides and fades along with the rest of the page
   const auto map_color = sf::Color{255, 255, 255, static_cast<uint8_t>(_alpha * 255.0f)};
   const auto map_position = sf::Vector2f{map_viewport_x_px + _move_offset_px, map_viewport_y_px};

#ifdef __EMSCRIPTEN__
   sf::Sprite map_sprite;
   map_sprite.position = map_position;
   map_sprite.textureRect = sf::FloatRect{{0.0f, 0.0f}, {map_viewport_width_px, map_viewport_height_px}};
   map_sprite.color = map_color;
   states.texture = &_level_render_texture->getTexture();
   window.draw(map_sprite, states);
#else
   sf::Sprite map_sprite(_level_render_texture->getTexture());
   map_sprite.setPosition(map_position);
   map_sprite.setColor(map_color);
   window.draw(map_sprite, states);
#endif
}

void IngameMenuMap::update(const sf::Time& dt)
{
   CameraPanorama::getInstance().update();

   _blink_time_s += dt.asSeconds();

   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft ||
            _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }
}

void IngameMenuMap::show()
{
   // open the map centered on the player instead of wherever it was left last time
   _pan_world_px = {};
   InGameMenuPage::show();
}

void IngameMenuMap::keyboardKeyPressed(sf::Keyboard::Key key)
{
   // a and s zoom in and out, matching the keyboard prompts in the map footer
   if (key == sf::Keyboard::Key::A)
   {
      zoom(-1);
   }
   else if (key == sf::Keyboard::Key::S)
   {
      zoom(1);
   }
}

void IngameMenuMap::zoom(int32_t direction)
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      return;
   }

   const auto detail_level_count = static_cast<int32_t>(level->getLevelMap().getDetailLevelCount());
   if (detail_level_count == 0)
   {
      return;
   }

   _zoom_level = std::clamp(_zoom_level + direction, 0, detail_level_count - 1);
   updateButtons();
}

void IngameMenuMap::pan(const sf::Vector2f& direction)
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      return;
   }

   const auto& level_map = level->getLevelMap();
   if (!level_map.isValid())
   {
      return;
   }

   // the pan offset is kept in world pixels so it survives a change of the detail level. one key
   // press always moves the view by the same amount on screen.
   const auto detail_level = static_cast<size_t>(_zoom_level);
   const auto world_px_per_map_px = level_map.getWorldPixelsPerMapPixel(detail_level);
   _pan_world_px += direction * pan_step_px * world_px_per_map_px;

   // clamp the whole view rectangle to the level, not just its center, so panning always keeps
   // level content on screen. axes on which the level is smaller than the view stay centered.
   const auto player_position_px = PlayerRegistry::getFirst()->getPixelPositionFloat();
   const auto map_size = level_map.getSize(detail_level);
   const auto level_size_px = sf::Vector2f{static_cast<float>(map_size.x), static_cast<float>(map_size.y)} * world_px_per_map_px;
   const auto view_size_px = sf::Vector2f{map_viewport_width_px, map_viewport_height_px} * world_px_per_map_px;

   const auto clamp_axis = [](float pan, float player_position, float level_size, float view_size)
   {
      if (view_size >= level_size)
      {
         return level_size * 0.5f - player_position;
      }
      return std::clamp(pan, view_size * 0.5f - player_position, level_size - view_size * 0.5f - player_position);
   };

   _pan_world_px.x = clamp_axis(_pan_world_px.x, player_position_px.x, level_size_px.x, view_size_px.x);
   _pan_world_px.y = clamp_axis(_pan_world_px.y, player_position_px.y, level_size_px.y, view_size_px.y);
}

void IngameMenuMap::left()
{
   pan({-1.0f, 0.0f});
}

void IngameMenuMap::right()
{
   pan({1.0f, 0.0f});
}

void IngameMenuMap::up()
{
   pan({0.0f, -1.0f});
}

void IngameMenuMap::down()
{
   pan({0.0f, 1.0f});
}

void IngameMenuMap::updateMove()
{
   const auto move_offset = getMoveOffset();

   // the map slides and fades with the rest of the page instead of popping in and out
   _move_offset_px = move_offset.value_or(0.0f);
   const auto screen_width = static_cast<float>(GameConfiguration::getInstance()._view_width);
   _alpha = std::clamp(1.0f - std::fabs(_move_offset_px) / screen_width, 0.0f, 1.0f);

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   for (const auto& layer : _panel_center)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   if (!move_offset.has_value())
   {
      _animation.reset();
   }
}

void IngameMenuMap::setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors)
{
   _doors = doors;
}

void IngameMenuMap::setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals)
{
   _portals = portals;
}

void IngameMenuMap::updateButtons()
{
   // show the keyboard prompts unless a controller is plugged in, otherwise the a/s zoom keys and
   // the arrow keys are nowhere to be seen
   const bool xbox = GameControllerIntegration::getInstance().isControllerConnected();
   bool close_enabled = false;
   bool legend_enabled = false;
   bool world_enabled = false;
   bool zoom_enabled = false;
   bool navigate_enabled = false;

   _layers["close_xbox_0"]->_visible = xbox;
   _layers["close_xbox_1"]->_visible = xbox && close_enabled;
   _layers["close_pc_0"]->_visible = !xbox;
   _layers["close_pc_1"]->_visible = !xbox && close_enabled;

   _layers["legend_xbox_0"]->_visible = xbox;
   _layers["legend_xbox_1"]->_visible = xbox && legend_enabled;
   _layers["legend_pc_0"]->_visible = !xbox;
   _layers["legend_pc_1"]->_visible = !xbox && legend_enabled;

   _layers["world_xbox_0"]->_visible = xbox;
   _layers["world_xbox_1"]->_visible = xbox && world_enabled;
   _layers["world_pc_0"]->_visible = !xbox;
   _layers["world_pc_1"]->_visible = !xbox && world_enabled;

   _layers["zoom_xbox_0"]->_visible = xbox;
   _layers["zoom_xbox_1"]->_visible = xbox && zoom_enabled;
   _layers["zoom_pc_0"]->_visible = !xbox;
   _layers["zoom_pc_1"]->_visible = !xbox && zoom_enabled;

   _layers["navigate_xbox_0"]->_visible = xbox;
   _layers["navigate_xbox_1"]->_visible = xbox && navigate_enabled;
   _layers["navigate_pc_0"]->_visible = !xbox;
   _layers["navigate_pc_1"]->_visible = !xbox && navigate_enabled;

   _layers["zoom_level_1"]->_visible = (_zoom_level == 0);
   _layers["zoom_level_2"]->_visible = (_zoom_level == 1);
   _layers["zoom_level_3"]->_visible = (_zoom_level == 2);
   _layers["zoom_level_4"]->_visible = (_zoom_level == 3);

   const auto next_menu = false;
   const auto prev_menu = false;

   _layers["next_menu_0"]->_visible = !next_menu;
   _layers["next_menu_1"]->_visible = next_menu;
   _layers["previous_menu_0"]->_visible = !prev_menu;
   _layers["previous_menu_1"]->_visible = prev_menu;
}

void IngameMenuMap::updateShowHide()
{
   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   sf::Vector2f panel_left_offset_px;
   sf::Vector2f panel_center_offset_px;
   sf::Vector2f panel_right_offset_px;

   auto& alpha = _alpha;
   alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < _duration_show.count())
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / _duration_show.count();
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f;

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = -150 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      panel_left_offset_px.x = 0;
      panel_center_offset_px.y = 0;
      panel_right_offset_px.x = 0;

      alpha = 1.0f;

      if (_animation == Animation::Show)
      {
         _animation.reset();
      }
   }

   // animate hide event
   if (_animation == Animation::Hide && duration_since_hide_s.count() < _duration_hide.count())
   {
      const auto elapsed_s_normalized = duration_since_hide_s.count() / _duration_hide.count();
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f);

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = -150 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = 1.0f - Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      if (_animation == Animation::Hide)
      {
         fullyHidden();
      }
   }

   // move in x
   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + panel_left_offset_px.x;
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + panel_right_offset_px.x;
      sfcompat::setPosition(*layer._layer->_sprite, {x, layer._pos.y});
   }

   // move in y
   for (const auto& layer : _panel_center)
   {
      const auto y = layer._pos.y + panel_center_offset_px.y;
      sfcompat::setPosition(*layer._layer->_sprite, {layer._pos.x, y});
   }

   // fade in/out
   for (const auto& layer : _panel_header)
   {
      sfcompat::setColor(*layer._layer->_sprite, sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }

   for (const auto& layer : _panel_background)
   {
      sfcompat::setColor(*layer._layer->_sprite, sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }
}

// window.draw(level_texture_sprite, sf::BlendMode{sf::BlendAdd});
//
// if (_zoom_enabled)
// {
// }
//
// if (DisplayMode::getInstance().isSet(Display::CameraPanorama))
// {
// }
//
// std::stringstream stream;
// auto pos = Player::getPlayer(0)->getPixelPosition();
// stream << "player pos: " << static_cast<int>(pos.x / TILE_WIDTH) << ", " << static_cast<int>(pos.y / TILE_HEIGHT);
//
// mFont.draw(window, mFont.getCoords(stream.str()), 5, 50);
// mFont.draw(window, mFont.getCoords(Console::getInstance().getCommand()), 5, 100);

// missing: map now has to get these on its own.
//
// _map->loadLevelTextures(path / std::filesystem::path("physics_grid_solid.png"), path / std::filesystem::path("physics_path_solid.png"));
// _map->setDoors(_mechanism_doors);
// _map->setPortals(_mechanism_portals);
