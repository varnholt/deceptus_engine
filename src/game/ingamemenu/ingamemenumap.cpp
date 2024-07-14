#include "ingamemenumap.h"

#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/camera/camerapanorama.h"
#include "game/config/gameconfiguration.h"
#include "game/debug/console.h"
#include "game/ingamemenu/menuconfig.h"
#include "game/mechanisms/door.h"
#include "game/mechanisms/portal.h"
#include "game/player/extratable.h"
#include "game/player/player.h"
#include "game/io/texturepool.h"

#include <iostream>
#include <sstream>

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
   _level_grid_sprite.setTexture(*_level_grid_texture);

   _level_outline_texture = TexturePool::getInstance().get(outlines.string());
   _level_outline_sprite.setTexture(*_level_outline_texture);

   // that render texture should have the same size as our level textures
   _level_render_texture.create(_level_grid_texture->getSize().x, _level_grid_texture->getSize().y);
}

void IngameMenuMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (_level_grid_texture)
   {
      sf::Vector2f center;
      center += Player::getCurrent()->getPixelPositionFloat() * 0.125f;
      center += CameraPanorama::getInstance().getLookVector();
      center.x += _level_grid_sprite.getTexture()->getSize().x / 2.0f;
      center.y += _level_grid_sprite.getTexture()->getSize().y / 2.0f;
      center.x -= 220.0f;
      center.y -= 80.0f;

      sf::View level_view;
      level_view.setSize(
         static_cast<float>(_level_grid_sprite.getTexture()->getSize().x), static_cast<float>(_level_grid_sprite.getTexture()->getSize().y)
      );

      level_view.setCenter(center);
      level_view.zoom(_zoom);  // 1.5f works well, too

      _level_grid_sprite.setColor(sf::Color{70, 70, 140, 255});
      _level_outline_sprite.setColor(sf::Color{255, 255, 255, 80});

      _level_render_texture.clear();
      _level_render_texture.draw(_level_grid_sprite, sf::BlendMode{sf::BlendAdd});
      _level_render_texture.draw(_level_outline_sprite, sf::BlendMode{sf::BlendAdd});

      drawLevelItems(_level_render_texture);

      _level_render_texture.setView(level_view);
      _level_render_texture.display();

      auto level_texture_sprite = sf::Sprite(_level_render_texture.getTexture());
      level_texture_sprite.move(10.0f, 48.0f);
   }

   InGameMenuPage::draw(window, states);
}

void IngameMenuMap::update(const sf::Time& /*dt*/)
{
   CameraPanorama::getInstance().update();

   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft || _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }
}

void IngameMenuMap::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_center)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
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

void IngameMenuMap::drawLevelItems(sf::RenderTarget& target, sf::RenderStates)
{
   float scale = 3.0f;

   // draw grid
   for (auto y = 0u; y < target.getSize().y; y += 16)
   {
      sf::Vertex a(sf::Vector2f(0.0f, static_cast<float>(y)));
      sf::Vertex b(sf::Vector2f(static_cast<float>(target.getSize().x), static_cast<float>(y)));
      a.color.a = 30;
      b.color.a = 30;

      sf::Vertex line[] = {a, b};

      target.draw(line, 2, sf::Lines);
   }

   for (auto x = 0u; x < target.getSize().x; x += 16)
   {
      sf::Vertex a(sf::Vector2f(static_cast<float>(x), 0.0f));
      sf::Vertex b(sf::Vector2f(static_cast<float>(x), static_cast<float>(target.getSize().y)));
      a.color.a = 30;
      b.color.a = 30;

      sf::Vertex line[] = {a, b};

      target.draw(line, 2, sf::Lines);
   }

   // draw doors
   float doorWidth = 2.0f;
   float doorHeight = 9.0f;

   for (auto& d : _doors)
   {
      auto door = std::dynamic_pointer_cast<Door>(d);

      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::White;
      quad[1].color = sf::Color::White;
      quad[2].color = sf::Color::White;
      quad[3].color = sf::Color::White;

      auto pos = sf::Vector2f(static_cast<float>(door->getTilePosition().x), static_cast<float>(door->getTilePosition().y));

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale), static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale + doorHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale), static_cast<float>(pos.y * scale + doorHeight));

      target.draw(&quad[0], 4, sf::Quads);
   }

   // draw portals
   float portalWidth = 3.0f;
   float portalHeight = 6.0f;
   for (auto& p : _portals)
   {
      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::Red;
      quad[1].color = sf::Color::Red;
      quad[2].color = sf::Color::Red;
      quad[3].color = sf::Color::Red;

      auto portal = std::dynamic_pointer_cast<Portal>(p);
      auto pos = sf::Vector2f(portal->getTilePosition().x, portal->getTilePosition().y);

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale), static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale + portalHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale), static_cast<float>(pos.y * scale + portalHeight));

      target.draw(&quad[0], 4, sf::Quads);
   }

   // draw player
   auto playerWidth = 5.0f;
   auto playerHeight = 4;
   sf::CircleShape square(playerWidth, static_cast<uint32_t>(playerHeight));
   square.setPosition(Player::getCurrent()->getPixelPositionFloat() * 0.125f);
   square.move(-playerWidth, -playerHeight * 2.0f);
   square.setFillColor(sf::Color::White);
   target.draw(square);
}

void IngameMenuMap::updateButtons()
{
   bool xbox = true;
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

   _layers["zoom_level_1"]->_visible = true;
   _layers["zoom_level_2"]->_visible = false;
   _layers["zoom_level_3"]->_visible = false;
   _layers["zoom_level_4"]->_visible = false;

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

   auto alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < _duration_show.count())
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / _duration_show.count();
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f;

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
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f);

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
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + panel_right_offset_px.x;
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // move in y
   for (const auto& layer : _panel_center)
   {
      const auto y = layer._pos.y + panel_center_offset_px.y;
      layer._layer->_sprite->setPosition(layer._pos.x, y);
   }

   // fade in/out
   for (const auto& layer : _panel_header)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }

   for (const auto& layer : _panel_background)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }
}

// window.draw(level_texture_sprite, sf::BlendMode{sf::BlendAdd});
//
// if (_zoom_enabled)
// {
// }
//
// if (CameraPanorama::getInstance().isLookActive())
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
