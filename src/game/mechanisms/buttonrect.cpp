#include "buttonrect.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismobserver.h"
#include "game/player/player.h"

ButtonRect::ButtonRect(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(GameNode).name());
}

void ButtonRect::update(const sf::Time& /*dt*/)
{
   _player_intersects = Player::getCurrent()->getPixelRectFloat().intersects(_rect);

   if (!_player_intersects)
   {
      return;
   }

   if ((_botton == Button::A && Player::getCurrent()->getControls()->isButtonAPressed()) ||
       (_botton == Button::B && Player::getCurrent()->getControls()->isButtonBPressed()) ||
       (_botton == Button::X && Player::getCurrent()->getControls()->isButtonXPressed()) ||
       (_botton == Button::Y && Player::getCurrent()->getControls()->isButtonYPressed()))
   {
      GameMechanismObserver::onEvent(getObjectId(), "button_rects", "pressed", "true");
   }
}

std::optional<sf::FloatRect> ButtonRect::getBoundingBoxPx()
{
   return _rect;
}

void ButtonRect::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   _rect = sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   if (data._tmx_object->_properties)
   {
      static std::unordered_map<std::string, Button> button_map{{"a", Button::A}, {"b", Button::B}, {"x", Button::X}, {"y", Button::Y}};

      const auto& map = data._tmx_object->_properties->_map;
      const auto button_id_str = ValueReader::readValue<std::string>("button", map).value_or("b");
   }

   addChunks(_rect);
}
