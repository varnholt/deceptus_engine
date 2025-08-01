#include "controllerhelp.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/controllerkeymap.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

// clang-format off
//   +---------+-------+-------+---------+--------+--------+----------+-----+-----+------------+-------------+--------------+-------------+-----+------+-----+
//   | 0       | 1     | 2     | 3       | 4      | 5      | 6        | 7   | 8   | 9          | 10          | 11           | 12          | 13  | 14   | 15  |
//   +---------+-------+-------+---------+--------+--------+----------+-----+-----+------------+-------------+--------------+-------------+-----+------+-----+
// 0 |key_esc  |key_1  |key_2  |key_3    |key_4   |key_5   |key_6     |key_7|key_8|key_9       |key_minus    |key_equals    |key_backspace|     |      |     |
// 1 |key_tab  |key_q  |key_w  |key_e    |key_r   |key_t   |key_y     |key_u|key_i|key_o       |key_p        |key_bracket_l |key_bracket_r|     |      |     |
// 2 |key_caps |key_a  |key_s  |key_d    |key_f   |key_g   |key_h     |key_j|key_k|key_l       |key_semicolon|key_apostrophe|key_return   |     |      |     |
// 3 |key_shift|key_0  |key_z  |key_x    |key_c   |key_v   |key_b     |key_n|key_m|key_comma   |key_period   |key_question  |key_backslash|     |      |     |
// 4 |key_ctrl |key_win|key_alt|key_empty|key_list|        |          |     |     |key_cursor_l|key_cursor_u |key_cursor_d  |key_cursor_r |     |      |     |
// 5 |bt_a     |bt_b   |bt_x   |bt_y     |bt_list |bt_menu |bt_rt     |bt_lt|bt_lb|bt_rb       |             |              |             |     |      |     |
// 6 |dpad_u   |dpad_d |dpad_l |dpad_r   |bt_u    |bt_d    |bt_l      |bt_r |bt_1 |bt_2        |bt_3         |bt_4          |bt_5         |bt_6 | bt_7 |bt_8 |
// 7 |bt_r_u   |bt_r_d |bt_r_l |bt_r_r   |bt_r_u_d|bt_r_l_r|dpad_empty|bt_0 |bt_9 |bt_10       |bt_11        |bt_12         |bt_13        |bt_14| bt_15|bt_16|
// 8 |bt_l_u   |bt_l_d |bt_l_l |bt_l_r   |bt_l_u_d|bt_l_l_r|key_door  |     |     |            |             |              |             |     |      |     |
//   +---------+-------+-------+---------+--------+--------+----------+-----+-----+------------+-------------+--------------+-------------+-----+------+-----+
// clang-format on

namespace
{
const auto registered_controllerhelp = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();

   registry.mapGroupToLayer("ControllerHelp", "controller_help");

   registry.registerLayerName(
      "controller_help",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ControllerHelp>(parent);
         mechanism->deserialize(data);
         mechanisms["controller_help"]->push_back(mechanism);
      }
   );

   registry.registerObjectGroup(
      "ControllerHelp",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ControllerHelp>(parent);
         mechanism->deserialize(data);
         mechanisms["controller_help"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
constexpr auto show_speed = 1.0f;
constexpr auto hide_speed = 1.0f;
constexpr auto alpha_min_threshold = 0.01f;
}  // namespace

ControllerHelp::ControllerHelp(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(ControllerHelp).name());
}

std::string_view ControllerHelp::objectName() const
{
   return "ControllerHelp";
}

void ControllerHelp::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_visible && _alpha <= alpha_min_threshold)
   {
      return;
   }

   // +--------------+--------------+
   //              [xxx]
   //           [xxx] [xxx]

   const auto color = sf::Color(255, 255, 255, static_cast<uint8_t>(_alpha * 255));
   const auto margin_x_px = (_sprites.size() - 1) * PIXELS_PER_TILE / 2.0f;
   const auto width_of_tiles_px = _sprites.size() * PIXELS_PER_TILE + margin_x_px;
   const auto tile_offset_y = sin(_time.asSeconds() * 5.0f) * 8.0f;

   // draw background
   _background->setPosition({_rect_center.x - _background->getTextureRect().size.x / 2, _rect_center.y + tile_offset_y - 11});
   _background->setColor(color);
   target.draw(*_background);

   const auto is_controller_connected = GameControllerIntegration::getInstance().isControllerConnected();

   // draw icons
   auto index = 0;
   for (auto& sprite : _sprites)
   {
      sprite.setColor(color);
      const auto tile_offset_x = -width_of_tiles_px / 2.0f + index * PIXELS_PER_TILE * 1.5f;
      sprite.setPosition({_rect_center.x + tile_offset_x, _rect_center.y + tile_offset_y});
      sprite.setTextureRect(is_controller_connected ? _sprite_rects_controller[index] : _sprite_rects_keyboard[index]);
      target.draw(sprite);
      index++;
   }
}

void ControllerHelp::update(const sf::Time& delta_time)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   _visible = (player_rect.findIntersection(_rect_px)).has_value();

   if (!_visible)
   {
      _alpha = std::max(0.0f, _alpha - delta_time.asSeconds() * hide_speed);

      // keep animating until fully disappeared
      if (_alpha > alpha_min_threshold)
      {
         _time += delta_time;
      }
   }
   else
   {
      _alpha = std::min(1.0f, _alpha + delta_time.asSeconds() * show_speed);
      _time += delta_time;
   }
}

void ControllerHelp::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   setZ(static_cast<int32_t>(ZDepth::Player) - 1);

   _rect_px =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   _rect_center = sf::Vector2f{
      data._tmx_object->_x_px + data._tmx_object->_width_px / 2.0f,
      data._tmx_object->_y_px + data._tmx_object->_height_px / 2.0f,
   };

   std::vector<std::string> buttons_unmapped;

   if (!data._tmx_object->_properties)
   {
      return;
   }

   auto keys_it = data._tmx_object->_properties->_map.find("keys");
   if (keys_it != data._tmx_object->_properties->_map.end())
   {
      std::istringstream text_line(keys_it->second->_value_string.value());
      std::string key;
      while (getline(text_line, key, ';'))
      {
         buttons_unmapped.emplace_back(key);
      }
   }

   _texture = TexturePool::getInstance().get("data/game/ui_icons.png");

   for (const auto& unmapped_button_key : buttons_unmapped)
   {
      const auto button_key_pair = ControllerKeyMap::retrieveMappedKey(unmapped_button_key);
      const auto pos_index_keyboard = ControllerKeyMap::getArrayPosition(button_key_pair.first);
      const auto pos_index_controller = ControllerKeyMap::getArrayPosition(button_key_pair.second);

      const auto sprite_rect_keyboard = sf::IntRect{
         {pos_index_keyboard.first * PIXELS_PER_TILE, pos_index_keyboard.second * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}
      };

      const auto sprite_rect_controller = sf::IntRect{
         {pos_index_controller.first * PIXELS_PER_TILE, pos_index_controller.second * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}
      };

      sf::Sprite sprite(*_texture);
      sprite.setTextureRect(sprite_rect_keyboard);
      _sprites.emplace_back(sprite);
      _sprite_rects_controller.emplace_back(sprite_rect_controller);
      _sprite_rects_keyboard.emplace_back(sprite_rect_keyboard);
   }

   _background = std::make_unique<sf::Sprite>(*_texture);

   if (_sprites.size() == 1)
   {
      _background->setTextureRect({{6 * PIXELS_PER_TILE, 10 * PIXELS_PER_TILE}, {PIXELS_PER_TILE * 2, PIXELS_PER_TILE * 2}});
   }
   else if (_sprites.size() == 2)
   {
      _background->setTextureRect({{9 * PIXELS_PER_TILE, 10 * PIXELS_PER_TILE}, {PIXELS_PER_TILE * 3, PIXELS_PER_TILE * 3}});
   }
}

std::optional<sf::FloatRect> ControllerHelp::getBoundingBoxPx()
{
   return _rect_px;
}
