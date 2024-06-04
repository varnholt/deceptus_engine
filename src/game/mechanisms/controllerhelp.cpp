#include "controllerhelp.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/gamecontrollerintegration.h"
#include "game/mechanisms/controllerkeymap.h"
#include "game/player/player.h"
#include "game/texturepool.h"

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
constexpr auto show_speed = 1.0f;
constexpr auto hide_speed = 1.0f;
}  // namespace

ControllerHelp::ControllerHelp(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(ControllerHelp).name());
}

void ControllerHelp::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_visible && _alpha <= 0.01f)
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
   _background.setPosition(_rect_center.x - _background.getTextureRect().width / 2, _rect_center.y + tile_offset_y - 11);
   _background.setColor(color);
   target.draw(_background);

   // draw icons
   auto index = 0;
   for (auto& sprite : _sprites)
   {
      sprite.setColor(color);
      const auto tile_offset_x = -width_of_tiles_px / 2.0f + index * PIXELS_PER_TILE * 1.5f;
      sprite.setPosition(_rect_center.x + tile_offset_x, _rect_center.y + tile_offset_y);
      target.draw(sprite);
      index++;
   }
}

void ControllerHelp::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   _visible = (player_rect.intersects(_rect_px));

   if (!_visible)
   {
      _alpha = std::max(0.0f, _alpha - dt.asSeconds() * hide_speed);

      // keep animating until fully disappeared
      if (_alpha > 0.01f)
      {
         _time += dt;
      }
   }
   else
   {
      _alpha = std::min(1.0f, _alpha + dt.asSeconds() * show_speed);
      _time += dt;
   }
}

void ControllerHelp::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   setZ(static_cast<int32_t>(ZDepth::Player) - 1);

   _rect_px = sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   _rect_center = sf::Vector2f{
      data._tmx_object->_x_px + data._tmx_object->_width_px / 2.0f,
      data._tmx_object->_y_px + data._tmx_object->_height_px / 2.0f,
   };

   std::vector<std::string> keys;
   auto keys_it = data._tmx_object->_properties->_map.find("keys");
   if (keys_it != data._tmx_object->_properties->_map.end())
   {
      _texture = TexturePool::getInstance().get("data/game/ui_icons.png");

      std::istringstream f(keys_it->second->_value_string.value());
      std::string key;
      while (getline(f, key, ';'))
      {
         // instead of the code below, have two sets of keys
         // - one for the controller
         // - one for the keyboard
         // and then just hold two sprite vectors

         // if the game controller is used, try to find a mapping
         if (GameControllerIntegration::getInstance().isControllerConnected())
         {
            const auto mapped_it = ControllerKeyMap::key_controller_map.find(key);
            if (mapped_it != ControllerKeyMap::key_controller_map.end())
            {
               key = mapped_it->second;
            }
         }

         keys.push_back(key);
      }
   }

   for (const auto& key : keys)
   {
      const auto pos_index = ControllerKeyMap::getArrayPosition(key);

      sf::Sprite sprite;
      sprite.setTexture(*_texture);
      sprite.setTextureRect({pos_index.first * PIXELS_PER_TILE, pos_index.second * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});

      _sprites.push_back(sprite);
   }

   _background.setTexture(*_texture);

   if (_sprites.size() == 1)
   {
      _background.setTextureRect({6 * PIXELS_PER_TILE, 10 * PIXELS_PER_TILE, PIXELS_PER_TILE * 2, PIXELS_PER_TILE * 2});
   }
   else if (_sprites.size() == 2)
   {
      _background.setTextureRect({9 * PIXELS_PER_TILE, 10 * PIXELS_PER_TILE, PIXELS_PER_TILE * 2, PIXELS_PER_TILE * 3});
   }
}

std::optional<sf::FloatRect> ControllerHelp::getBoundingBoxPx()
{
   return _rect_px;
}
