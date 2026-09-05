#include "ingamemenumap.h"

#include "framework/easings/easings.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/camera/camerapanorama.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/debug/console.h"
#include "game/ingamemenu/ingamemenulabels.h"
#include "game/ingamemenu/menuconfig.h"
#include "game/io/texturepool.h"
#include "game/level/levelmap.h"
#include "game/level/levelregistry.h"
#include "game/level/room.h"
#include "game/mechanisms/door.h"
#include "game/mechanisms/portal.h"
#include "game/player/playerregistry.h"
#include "game/ui/menulabel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace
{

//! area of the map page that shows the level, in menu pixels. everything outside is header,
//! footer and the panels that slide in from the sides.
constexpr auto map_viewport_x_px = 10.0f;
constexpr auto map_viewport_y_px = 42.0f;
constexpr auto map_viewport_width_px = 620.0f;
constexpr auto map_viewport_height_px = 290.0f;

//! top panning speed in menu pixels per second, reached while a direction is held down
constexpr auto pan_max_speed_px_per_s = 360.0f;

//! time it takes to ease from standstill to the top speed and back again
constexpr auto pan_acceleration_s = 0.35f;
constexpr auto pan_deceleration_s = 0.2f;

//! analog stick deflection below this is treated as no input at all
constexpr auto pan_dead_zone = 0.2f;

//! multiplied onto the map texture for rooms that a map item revealed but the player has not
//! entered yet. it dims and warms the blue palette, so revealed and visited read as clearly
//! different without needing a second set of textures.
constexpr auto unvisited_tint = sf::Color{150, 128, 116};

constexpr auto blink_interval_s = 1.0f;

//! the magenta pill of the tab strip sits behind 'Map' in this page's artwork
constexpr auto header_pill_left_px = 0;
constexpr auto header_pill_width_px = 73;

//! columns each button icon of the footer occupies inside its own layer image
constexpr auto icon_width_navigate_pc_px = 23;
constexpr auto icon_width_navigate_xbox_px = 18;
constexpr auto icon_width_zoom_pc_px = 34;
constexpr auto icon_width_zoom_xbox_px = 18;
constexpr auto icon_width_world_pc_px = 16;
constexpr auto icon_width_world_xbox_px = 18;
constexpr auto icon_width_legend_pc_px = 16;
constexpr auto icon_width_legend_xbox_px = 16;
constexpr auto icon_width_close_pc_px = 16;
constexpr auto icon_width_close_xbox_px = 12;

//! columns the two badges of the legend occupy inside the 'map_keys' layer image
constexpr auto legend_icon_width_px = 11;

//! rows of the legend layer image the first entry occupies; the second follows below
constexpr auto legend_row_height_px = 13;
constexpr auto legend_row_stride_px = 16;

//! rows of the zone name layer holding the name; everything below is the flourish under it
constexpr auto zone_name_band_height_px = 17;

const sf::Color color_legend_teleport{0, 198, 221};
const sf::Color color_legend_checkpoint{0, 221, 16};
const sf::Color color_zone_name{160, 173, 203};

//! cell order inside the marker spriteset layers
constexpr auto marker_index_player = 0;
constexpr auto marker_index_checkpoint = 1;
constexpr auto marker_index_portal = 2;
constexpr auto marker_index_door = 3;

//! one marker icon per zoom step, matching the zoom_level_* layers
constexpr auto marker_detail_level_count = 4u;

}  // namespace

IngameMenuMap::IngameMenuMap()
{
   _font.load("data/game/font.png", "data/game/font.map");
   _filename = "data/game/map.psd";

   load();
   updateLabels();
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

   collectMarkerLayers();

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;
}

void IngameMenuMap::updateLabels()
{
   InGameMenuLabels::updateHeaderLabels(*_layers["header"], InGameMenuLabels::Tab::Map, header_pill_left_px, header_pill_width_px);

   // both rows are laid out, since only one of them is ever visible and the page switches between
   // them as a controller comes and goes
   InGameMenuLabels::updateFooterLabels({
      {._layer_plain = _layers["navigate_xbox_0"].get(),
       ._layer_pressed = _layers["navigate_xbox_1"].get(),
       ._icon_width_px = icon_width_navigate_xbox_px,
       ._text = "Navigate"},
      {._layer_plain = _layers["zoom_xbox_0"].get(),
       ._layer_pressed = _layers["zoom_xbox_1"].get(),
       ._icon_width_px = icon_width_zoom_xbox_px,
       ._text = "Zoom"},
      {._layer_plain = _layers["world_xbox_0"].get(),
       ._layer_pressed = _layers["world_xbox_1"].get(),
       ._icon_width_px = icon_width_world_xbox_px,
       ._text = "World Map"},
      {._layer_plain = _layers["legend_xbox_0"].get(),
       ._layer_pressed = _layers["legend_xbox_1"].get(),
       ._icon_width_px = icon_width_legend_xbox_px,
       ._text = "Legend"},
      {._layer_plain = _layers["close_xbox_0"].get(),
       ._layer_pressed = _layers["close_xbox_1"].get(),
       ._icon_width_px = icon_width_close_xbox_px,
       ._text = "Close"},
   });

   InGameMenuLabels::updateFooterLabels({
      {._layer_plain = _layers["navigate_pc_0"].get(),
       ._layer_pressed = _layers["navigate_pc_1"].get(),
       ._icon_width_px = icon_width_navigate_pc_px,
       ._text = "Navigate"},
      {._layer_plain = _layers["zoom_pc_0"].get(),
       ._layer_pressed = _layers["zoom_pc_1"].get(),
       ._icon_width_px = icon_width_zoom_pc_px,
       ._text = "Zoom"},
      {._layer_plain = _layers["world_pc_0"].get(),
       ._layer_pressed = _layers["world_pc_1"].get(),
       ._icon_width_px = icon_width_world_pc_px,
       ._text = "World Map"},
      {._layer_plain = _layers["legend_pc_0"].get(),
       ._layer_pressed = _layers["legend_pc_1"].get(),
       ._icon_width_px = icon_width_legend_pc_px,
       ._text = "Legend"},
      {._layer_plain = _layers["close_pc_0"].get(),
       ._layer_pressed = _layers["close_pc_1"].get(),
       ._icon_width_px = icon_width_close_pc_px,
       ._text = "Close"},
   });

   updateLegendLabels();
   updateZoneNameLabel();
}

void IngameMenuMap::updateLegendLabels()
{
   auto& legend_layer = *_layers["map_keys"];
   if (!legend_layer._texture)
   {
      return;
   }

   constexpr auto legend_text_x_px = 21;

   const auto layer_size = legend_layer._texture->getSize();
   const auto teleport_width_px = static_cast<int32_t>(std::ceil(MenuLabel::measure("Teleport", 12)));
   const auto checkpoint_width_px = static_cast<int32_t>(std::ceil(MenuLabel::measure("Checkpoint", 12)));
   const auto width_px = legend_text_x_px + std::max({teleport_width_px, checkpoint_width_px, 1});

   MenuLabel::compose(
      legend_layer,
      {width_px, static_cast<int32_t>(layer_size.y)},
      {MenuLabel::Piece{
          ._source = sf::IntRect{{0, 0}, {legend_icon_width_px, legend_row_height_px}},  //
          ._target = sf::Vector2i{0, 0}
       },
       MenuLabel::Piece{
          ._source = sf::IntRect{{0, legend_row_stride_px}, {legend_icon_width_px, legend_row_height_px}},
          ._target = sf::Vector2i{0, legend_row_stride_px}
       }},
      {MenuLabel::Label{
          ._text = "Teleport",
          ._box = sf::IntRect{{legend_text_x_px, 0}, {width_px - legend_text_x_px, legend_row_height_px}},
          ._align = MenuLabel::Align::Left,
          ._character_size = 12,
          ._color = color_legend_teleport
       },
       MenuLabel::Label{
          ._text = "Checkpoint",
          ._box = sf::IntRect{{legend_text_x_px, legend_row_stride_px}, {width_px - legend_text_x_px, legend_row_height_px}},
          ._align = MenuLabel::Align::Left,
          ._character_size = 12,
          ._color = color_legend_checkpoint
       }}
   );
}

void IngameMenuMap::updateZoneNameLabel()
{
   auto& zone_layer = *_layers["zone_name_label_crypts"];
   if (!zone_layer._texture || !zone_layer._sprite)
   {
      return;
   }

   // the zone name and the flourish under it are one image, so the flourish is kept and only the
   // band the name sat in is redrawn
   const auto layer_size = zone_layer._texture->getSize();
   const auto original_width_px = static_cast<int32_t>(layer_size.x);
   const auto height_px = static_cast<int32_t>(layer_size.y);
   const auto name_width_px = static_cast<int32_t>(std::ceil(MenuLabel::measure("The Forgotten Crypts", 12)));
   const auto width_px = std::max(original_width_px, name_width_px + 16);
   const auto flourish_x_px = (width_px - original_width_px) / 2;

   const auto position = sfcompat::getPosition(*zone_layer._sprite);

   MenuLabel::compose(
      zone_layer,
      {width_px, height_px},
      {MenuLabel::Piece{
         ._source = sf::IntRect{{0, zone_name_band_height_px}, {original_width_px, height_px - zone_name_band_height_px}},
         ._target = sf::Vector2i{flourish_x_px, zone_name_band_height_px}
      }},
      {MenuLabel::Label{
         ._text = "The Forgotten Crypts",
         ._box = sf::IntRect{{0, 0}, {width_px, zone_name_band_height_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = 12,
         ._color = color_zone_name
      }}
   );

   sfcompat::setPosition(*zone_layer._sprite, {position.x - static_cast<float>(flourish_x_px), position.y});
}

void IngameMenuMap::loadLevelTextures(const std::filesystem::path& grid, const std::filesystem::path& outlines)
{
   _level_grid_texture = TexturePool::getInstance().get(grid.string());
   _level_outline_texture = TexturePool::getInstance().get(outlines.string());

#ifdef DECEPTUS_VRSFML
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

#ifdef DECEPTUS_VRSFML
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

void IngameMenuMap::drawExploredRooms(const LevelMap& level_map, const sf::RenderStates& map_states)
{
   const auto level = LevelRegistry::getCurrent();
   const auto detail_level = static_cast<size_t>(_zoom_level);
   const auto revealed = level->isMapRevealed();

   // one quad per drawn sub-room, all textured from the same level map texture. the texture is
   // opaque, so overlapping sub-rooms simply write the same pixels twice.
   //
   // a map item reveals the rooms the player has not been to yet. those are tinted, so the layout
   // is readable without losing track of where the player has actually been.
   sf::VertexArray quads(sf::PrimitiveType::Triangles);

   for (const auto& room : level->getRooms())
   {
      for (const auto& sub_room : room->_sub_rooms)
      {
         if (!sub_room._visited && !revealed)
         {
            continue;
         }

         const auto color = sub_room._visited ? sf::Color::White : unvisited_tint;
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
         quads.append(sf::Vertex{top_left, color, top_left});
         quads.append(sf::Vertex{top_right, color, top_right});
         quads.append(sf::Vertex{bottom_right, color, bottom_right});

         quads.append(sf::Vertex{top_left, color, top_left});
         quads.append(sf::Vertex{bottom_right, color, bottom_right});
         quads.append(sf::Vertex{bottom_left, color, bottom_left});
      }
   }

   auto states = map_states;
   states.texture = level_map.getTexture(detail_level);
   _level_render_texture->draw(quads, states);
}

void IngameMenuMap::collectMarkerLayers()
{
   // the marker art lives in the map psd so it can be edited without touching code: one spriteset
   // layer per zoom step, holding a square cell per marker kind. the layers are parked inside the
   // canvas for the artist's convenience, so they are taken out of the page layer stack here and
   // stamped at the marker positions instead.
   for (auto detail_level = 0u; detail_level < marker_detail_level_count; detail_level++)
   {
      const auto layer_name = std::format("markers_{}", detail_level + 1);
      const auto& layer_it = _layers.find(layer_name);

      if (layer_it == _layers.end() || !layer_it->second)
      {
         Log::Warning() << "map is missing marker spriteset '" << layer_name << "'";
         continue;
      }

      layer_it->second->_visible = false;
      _marker_strips[detail_level] = layer_it->second;

      std::erase(_layer_stack, layer_it->second);
   }
}

void IngameMenuMap::drawMarker(
   sf::RenderTarget& target,
   int32_t marker_index,
   size_t detail_level,
   const sf::Vector2f& center_viewport_px,
   const sf::RenderStates& marker_states
)
{
   // fall back to the most detailed spriteset when the artist has not drawn one for this zoom step
   auto layer = (detail_level < _marker_strips.size()) ? _marker_strips[detail_level] : nullptr;
   if (!layer)
   {
      layer = _marker_strips[0];
   }

   if (!layer || !layer->_sprite)
   {
      return;
   }

   // the cells are square, so the strip height is the cell size and the code never has to know
   // how big the artist drew them. the size comes from the texture rather than from the sprite's
   // texture rect, because this function overwrites that rect to select a cell.
   if (!layer->_texture)
   {
      return;
   }

   const auto strip_size = layer->_texture->getSize();
   const auto cell_size = static_cast<int32_t>(strip_size.y);
   if (cell_size <= 0 || static_cast<uint32_t>((marker_index + 1) * cell_size) > strip_size.x)
   {
      return;
   }

#ifdef DECEPTUS_VRSFML
   sfcompat::setTextureRect(
      *layer->_sprite,
      sf::FloatRect{{static_cast<float>(marker_index * cell_size), 0.0f}, {static_cast<float>(cell_size), static_cast<float>(cell_size)}}
   );
#else
   sfcompat::setTextureRect(*layer->_sprite, sf::IntRect{{marker_index * cell_size, 0}, {cell_size, cell_size}});
#endif

   // the icons are odd sized, so half of their size is not a whole pixel. integer division keeps
   // the sprite on the pixel grid, a float halving would park it between two pixels.
   const auto position = sf::Vector2f{
      std::round(center_viewport_px.x) - static_cast<float>(cell_size / 2),
      std::round(center_viewport_px.y) - static_cast<float>(cell_size / 2)
   };

   sfcompat::setPosition(*layer->_sprite, position);
   layer->draw(target, marker_states);
}

void IngameMenuMap::drawMarkers(const LevelMap& level_map, const sf::RenderStates& marker_states, const sf::Vector2f& view_top_left_map_px)
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

   const auto draw_mechanisms = [this, &level_map, &is_discovered, detail_level, &marker_states, &view_top_left_map_px](
                                   const std::vector<std::shared_ptr<GameMechanism>>& mechanisms, int32_t marker_index
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
         drawMarker(
            *_level_render_texture,
            marker_index,
            detail_level,
            level_map.toMap(center_px, detail_level) - view_top_left_map_px,
            marker_states
         );
      }
   };

   const auto& mechanism_registry = level->getMechanismRegistry();
   draw_mechanisms(mechanism_registry.getPortals(), marker_index_portal);
   draw_mechanisms(mechanism_registry.getCheckpoints(), marker_index_checkpoint);
   draw_mechanisms(mechanism_registry.getDoors(), marker_index_door);

   // the player marker blinks so it stays visible against the map fill
   if (std::fmod(_blink_time_s, blink_interval_s) < blink_interval_s * 0.7f)
   {
      const auto player_position_px = PlayerRegistry::getFirst()->getPixelPositionFloat();
      drawMarker(
         *_level_render_texture,
         marker_index_player,
         detail_level,
         level_map.toMap(player_position_px, detail_level) - view_top_left_map_px,
         marker_states
      );
   }
}

void IngameMenuMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
#ifdef DECEPTUS_VRSFML
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

   // vrsfml has no RenderTexture::setView, there the view travels in the render states instead
   sf::RenderStates map_states;

#ifdef DECEPTUS_VRSFML
   map_states.view = sf::View{.center = center_map_px, .size = view_size};
#else
   sf::View map_view;
   map_view.setSize(view_size);
   map_view.setCenter(center_map_px);
#endif

   _level_render_texture->clear(sf::Color::Transparent);

#ifndef DECEPTUS_VRSFML
   _level_render_texture->setView(map_view);
#endif

   drawExploredRooms(level_map, map_states);

   // markers are screen space ui, so they are drawn in the render texture's own pixel space rather
   // than through the panning map view. that way their alignment cannot depend on where the map
   // view happens to sit, and an icon can never land between two pixels.
   const auto view_top_left_map_px = center_map_px - view_size * 0.5f;

   sf::RenderStates marker_states;
#ifdef DECEPTUS_VRSFML
   marker_states.view = sf::View{.center = view_size * 0.5f, .size = view_size};
#else
   _level_render_texture->setView(_level_render_texture->getDefaultView());
#endif

   drawMarkers(level_map, marker_states, view_top_left_map_px);

   _level_render_texture->display();

   // the map slides and fades along with the rest of the page
   const auto map_color = sf::Color{255, 255, 255, static_cast<uint8_t>(_alpha * 255.0f)};
   const auto map_position = sf::Vector2f{map_viewport_x_px + _move_offset_px, map_viewport_y_px};

#ifdef DECEPTUS_VRSFML
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
   else
   {
      // only pan while the page sits still, a page sliding out must not drift away as well
      updatePan(dt);
   }
}

void IngameMenuMap::show()
{
   // open the map centered on the player instead of wherever it was left last time
   _pan_world_px = {};
   _pan_direction = {};
   _pan_ramp = 0.0f;
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

sf::Vector2f IngameMenuMap::readPanInput() const
{
   // the returned vector points into the pan direction, its length is the requested fraction of
   // the top speed. digital input always asks for the full speed, the analog stick scales it.
   sf::Vector2f input;

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
   {
      input.x -= 1.0f;
   }
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
   {
      input.x += 1.0f;
   }
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
   {
      input.y -= 1.0f;
   }
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
   {
      input.y += 1.0f;
   }

   const auto& controller_integration = GameControllerIntegration::getInstance();
   if (controller_integration.isControllerConnected())
   {
      const auto& controller = controller_integration.getController();
      const auto& joystick_info = controller->getInfo();
      const auto& axis_values = joystick_info.getAxisValues();

      const auto axis_left_x = controller->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
      const auto axis_left_y = controller->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTY);

      if (axis_left_x >= 0 && axis_left_y >= 0 && static_cast<size_t>(axis_left_x) < axis_values.size() &&
          static_cast<size_t>(axis_left_y) < axis_values.size())
      {
         const auto stick = sf::Vector2f{axis_values[axis_left_x] / 32767.0f, axis_values[axis_left_y] / 32767.0f};
         const auto deflection = std::hypot(stick.x, stick.y);

         if (deflection > pan_dead_zone)
         {
            // rescale so the speed starts at zero right outside the dead zone instead of jumping
            const auto scaled = std::min(1.0f, (deflection - pan_dead_zone) / (1.0f - pan_dead_zone));
            input += (stick / deflection) * scaled;
         }
      }

      // the dpad is digital, so it asks for the full speed just like the keyboard does
      const auto& hat_values = joystick_info.getHatValues();
      if (!hat_values.empty())
      {
         const auto hat = hat_values.at(0);
         if (static_cast<bool>(hat & SDL_HAT_LEFT))
         {
            input.x -= 1.0f;
         }
         if (static_cast<bool>(hat & SDL_HAT_RIGHT))
         {
            input.x += 1.0f;
         }
         if (static_cast<bool>(hat & SDL_HAT_UP))
         {
            input.y -= 1.0f;
         }
         if (static_cast<bool>(hat & SDL_HAT_DOWN))
         {
            input.y += 1.0f;
         }
      }
   }

   const auto length = std::hypot(input.x, input.y);
   if (length <= 0.0f)
   {
      return {};
   }

   // diagonals must not be faster than the straight directions
   return (input / length) * std::min(1.0f, length);
}

void IngameMenuMap::updatePan(const sf::Time& dt)
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

   const auto input = readPanInput();
   const auto requested_speed = std::hypot(input.x, input.y);
   const auto elapsed_s = dt.asSeconds();

   // hold a direction and the view eases up to the top speed, let go and it eases back to a stop.
   // the ramp is a separate value from the direction so releasing keeps gliding the last way.
   if (requested_speed > 0.0f)
   {
      _pan_direction = input / requested_speed;
      _pan_ramp = std::min(1.0f, _pan_ramp + elapsed_s / pan_acceleration_s);
   }
   else
   {
      _pan_ramp = std::max(0.0f, _pan_ramp - elapsed_s / pan_deceleration_s);
   }

   if (_pan_ramp <= 0.0f)
   {
      return;
   }

   // the analog stick scales the top speed, digital input always requests all of it
   const auto scale = (requested_speed > 0.0f) ? requested_speed : 1.0f;
   const auto speed_px_per_s = pan_max_speed_px_per_s * Easings::easeInOutCubic(_pan_ramp) * scale;

   // the offset is kept in world pixels so it survives a change of the detail level
   const auto detail_level = static_cast<size_t>(_zoom_level);
   const auto world_px_per_map_px = level_map.getWorldPixelsPerMapPixel(detail_level);
   _pan_world_px += _pan_direction * speed_px_per_s * world_px_per_map_px * elapsed_s;

   clampPan(level_map);
}

void IngameMenuMap::clampPan(const LevelMap& level_map)
{
   // clamp the whole view rectangle to the level, not just its center, so panning always keeps
   // level content on screen. axes on which the level is smaller than the view stay centered.
   const auto detail_level = static_cast<size_t>(_zoom_level);
   const auto world_px_per_map_px = level_map.getWorldPixelsPerMapPixel(detail_level);
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

   const auto clamped_x = clamp_axis(_pan_world_px.x, player_position_px.x, level_size_px.x, view_size_px.x);
   const auto clamped_y = clamp_axis(_pan_world_px.y, player_position_px.y, level_size_px.y, view_size_px.y);

   // running into the level border stops the movement on that axis instead of grinding along it
   if (clamped_x != _pan_world_px.x)
   {
      _pan_world_px.x = clamped_x;
      _pan_direction.x = 0.0f;
   }

   if (clamped_y != _pan_world_px.y)
   {
      _pan_world_px.y = clamped_y;
      _pan_direction.y = 0.0f;
   }
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
