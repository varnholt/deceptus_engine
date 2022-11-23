#include "levelmap.h"

#include "camerapanorama.h"
#include "console.h"
#include "extratable.h"
#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "gameconfiguration.h"
#include "mechanisms/door.h"
#include "mechanisms/portal.h"
#include "player/player.h"
#include "texturepool.h"

#include <iostream>
#include <sstream>


LevelMap::LevelMap()
{
   _font.load(
      "data/game/font.png",
      "data/game/font.map"
   );

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/map.psd");

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      // Log::Info() << layer.getName();

      auto tmp = std::make_shared<Layer>();
      tmp->_visible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layers[layer.getName()] = tmp;
   }

   updateButtons();
}

void LevelMap::loadLevelTextures(const std::filesystem::path& grid, const std::filesystem::path& outlines)
{
   _level_grid_texture = TexturePool::getInstance().get(grid.string());
   _level_grid_sprite.setTexture(*_level_grid_texture);

   _level_outline_texture = TexturePool::getInstance().get(outlines.string());
   _level_outline_sprite.setTexture(*_level_outline_texture);

   // that render texture should have the same size as our level textures
   _level_render_texture.create(_level_grid_texture->getSize().x, _level_grid_texture->getSize().y);
}


void LevelMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   // draw map
   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;

   sf::Vector2f center;
   center += Player::getCurrent()->getPixelPositionFloat() * 0.125f;
   center += CameraPanorama::getInstance().getLookVector();
   center.x += _level_grid_sprite.getTexture()->getSize().x / 2.0f;
   center.y += _level_grid_sprite.getTexture()->getSize().y / 2.0f;
   center.x -= 220.0f;
   center.y -= 80.0f;

   sf::View level_view;
   level_view.setSize(static_cast<float>(_level_grid_sprite.getTexture()->getSize().x), static_cast<float>(_level_grid_sprite.getTexture()->getSize().y));
   level_view.setCenter(center);
   level_view.zoom(_zoom); // 1.5f works well, too
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

   // draw layers
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   for (auto [k, v] : _layers)
   {
      if (!v->_visible)
      {
         continue;
      }
      v->draw(window, states);
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
}


void LevelMap::setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors)
{
   _doors = doors;
}


void LevelMap::setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals)
{
   _portals = portals;
}


void LevelMap::drawLevelItems(sf::RenderTarget& target, sf::RenderStates)
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

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale),             static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale + doorHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale),             static_cast<float>(pos.y * scale + doorHeight));

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

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale),               static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale + portalHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale),               static_cast<float>(pos.y * scale + portalHeight));

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

void LevelMap::updateButtons()
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

   // cpan_bg
   // cpan_right
   // cpan_left
   // cpan_down
   // cpan_up
   //
   // bg
   //
   // footer_bg
   // header_bg
   //
   // map_keys
   // zone_name_label_crypts
   // header
   //
   // next_menu_0
   // next_menu_1
   // previous_menu_0
   // previous_menu_1
}
