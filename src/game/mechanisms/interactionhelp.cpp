#include "interactionhelp.h"

#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/mechanisms/controllerkeymap.h"
#include "game/player/player.h"
#include "game/texturepool.h"
#include "game/valuereader.h"

InteractionHelp::InteractionHelp(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(InteractionHelp).name());

   if (_font.loadFromFile("data/fonts/deceptum.ttf"))
   {
      const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);
      _text.setFont(_font);
      _text.setCharacterSize(12);
   }
}

void InteractionHelp::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!isEnabled())
   {
      return;
   }

   _animation_show->draw(target);
   _animation_hide->draw(target);

   if (_button_alpha.has_value())
   {
      const auto level_view = target.getView();

      const sf::View ortho(sf::FloatRect(
         0.0f,
         0.0f,
         static_cast<float>(GameConfiguration::getInstance()._view_width),
         static_cast<float>(GameConfiguration::getInstance()._view_height)
      ));

      target.setView(ortho);
      target.draw(_button_sprite);
      target.draw(_text);
      target.setView(level_view);
   }
}

// workflow
// 1) on intersection
//    if hide is not playing
//    play show
// 2) show last show frame (show is paused)
// 3) if show is paused
//    if no intersection
//    play hide

void InteractionHelp::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      return;
   }

   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto intersects = player_rect.intersects(_rect_px);

   if (intersects && !_player_intersected_in_last_frame && _animation_hide->_paused)
   {
      // show
      _animation_hide->setVisible(false);
      _animation_show->setVisible(true);

      _animation_show->seekToStart();
      _animation_show->play();
      _active = true;
   }
   else if (!intersects && _animation_show->_paused && _active)
   {
      // hide
      _animation_show->setVisible(false);
      _animation_hide->setVisible(true);

      _animation_hide->seekToStart();
      _animation_hide->play();
      _active = false;
   }

   // update alpha
   std::optional<float> alpha;

   if (!_animation_show->_paused)
   {
      _animation_show->update(dt);
      alpha = static_cast<float>(_animation_show->_current_frame) / static_cast<float>(_animation_show->_frames.size());
   }

   if (!_animation_hide->_paused)
   {
      _animation_hide->update(dt);
      alpha = 1.0f - (static_cast<float>(_animation_hide->_current_frame) / static_cast<float>(_animation_show->_frames.size()));
   }

   if (_active && _animation_show->_paused)
   {
      alpha = 1.0f;
   }

   _button_alpha = alpha;
   if (alpha.has_value())
   {
      const auto alpha_byte = static_cast<uint8_t>(alpha.value() * 255);
      _text.setFillColor(sf::Color{232, 219, 243, alpha_byte});
      _button_sprite.setColor({255, 255, 255, alpha_byte});
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      _button_sprite.setTextureRect(_button_rect_controller);
   }
   else
   {
      _button_sprite.setTextureRect(_button_rect_keyboard);
   }

   _player_intersected_in_last_frame = intersects;
}

void InteractionHelp::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _rect_px = sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   const auto& map = data._tmx_object->_properties->_map;
   setZ(ValueReader::readValue<int32_t>("z", map).value_or(static_cast<int32_t>(ZDepth::ForegroundMax)));

   // read animations if set up
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;

   AnimationPool animation_pool{"data/sprites/interaction_animations.json"};
   const auto animation = ValueReader::readValue<std::string>("animation", map);
   if (animation.has_value())
   {
      const auto offset_x_px = ValueReader::readValue<int32_t>("offset_x_px", map).value_or(0);
      const auto offset_y_px = ValueReader::readValue<int32_t>("offset_y_px", map).value_or(0);

      _animation_show = animation_pool.create(
         animation.value(), pos_x_px + static_cast<float>(offset_x_px), pos_y_px + static_cast<float>(offset_y_px), false, false
      );
      _animation_show->_looped = false;
      _animation_show->_reset_to_first_frame = false;

      _animation_hide = animation_pool.create(
         animation.value(), pos_x_px + static_cast<float>(offset_x_px), pos_y_px + static_cast<float>(offset_y_px), false, false
      );
      _animation_hide->_looped = false;
      _animation_hide->_reset_to_first_frame = false;
      _animation_hide->reverse();
   }

   _button_texture = TexturePool::getInstance().get("data/game/ui_icons.png");
   _button_sprite.setTexture(*_button_texture);

   // read button icon
   const auto button_name = ValueReader::readValue<std::string>("button", map).value_or("key_cursor_u");
   const auto button_names_keyboard_controller = ControllerKeyMap::retrieveMappedKey(button_name);
   const auto pos_index_keyboard = ControllerKeyMap::getArrayPosition(button_names_keyboard_controller.first);
   const auto pos_index_controller = ControllerKeyMap::getArrayPosition(button_names_keyboard_controller.second);

   _button_rect_keyboard = {pos_index_keyboard.first * PIXELS_PER_TILE, pos_index_keyboard.second * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};
   _button_rect_controller = {pos_index_controller.first * PIXELS_PER_TILE, pos_index_controller.second * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};
   _button_sprite.setTextureRect(_button_rect_keyboard);
   _button_sprite.setPosition(550, 335);

   const auto text = ValueReader::readValue<std::string>("text", map).value_or("");
   _text.setString(text.c_str());
   _text.setPosition(574, 339);
}

std::optional<sf::FloatRect> InteractionHelp::getBoundingBoxPx()
{
   return _rect_px;
}
