#include "extra.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/audio.h"
#include "game/constants.h"
#include "game/extratable.h"
#include "game/gamedeserializedata.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/texturepool.h"

#include <iostream>

Extra::Extra(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Extra).name());
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::deserialize(const GameDeserializeData& data)
{
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   std::cout << "extra at: " << pos_x_px << ", " << pos_y_px << " (width: " << width_px << ", height: " << height_px << ")" << std::endl;

   _name = data._tmx_object->_name;
   _rect = {pos_x_px, pos_y_px, width_px, height_px};

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         _z = z_index;
      }

      const auto texture_it = data._tmx_object->_properties->_map.find("texture");
      if (texture_it != data._tmx_object->_properties->_map.end())
      {
         const auto texture_path = texture_it->second->_value_string.value();
         _texture = TexturePool::getInstance().get(texture_path);
         _sprite.setTexture(*_texture);
         _sprite.setPosition(pos_x_px, pos_y_px);
      }

      const auto sample_it = data._tmx_object->_properties->_map.find("sample");
      if (sample_it != data._tmx_object->_properties->_map.end())
      {
         _sample = sample_it->second->_value_string.value();
      }
   }

   // add
   // - enable/disable mechanism function to level
   // - add enable/disable mechanism code to levelscript
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (!_active)
   {
      return;
   }

   target.draw(_sprite);
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::update(const sf::Time& /*dt*/)
{
   if (!_active)
   {
      return;
   }

   const auto& player_rect_px = Player::getCurrent()->getPixelRectFloat();

   // std::cout << "x: " << _rect.left << ", " << _rect.top << " (width: " << _rect.width << ", height: " << _rect.height << ")"
   //           << " vs "
   //           << "x: " << player_rect_px.left << ", " << player_rect_px.top << " (width: " << player_rect_px.width
   //           << ", height: " << player_rect_px.height << ")" << std::endl;

   if (player_rect_px.intersects(_rect))
   {
      _active = false;

      for (auto& cb : _callbacks)
      {
         cb(_name);
      }

      if (_sample.has_value())
      {
         Audio::getInstance().playSample({_sample.value()});
      }

      // case ExtraItem::ExtraSpriteIndex::KeyYellow:
      // {
      //    SaveState::getPlayerInfo()._inventory.add(ItemType::KeyYellow);
      //    break;
      // }
      // case ExtraItem::ExtraSpriteIndex::Dash:
      // {
      //    SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Dash);
      //    break;
      // }
   }
}

std::optional<sf::FloatRect> Extra::getBoundingBoxPx()
{
   return _rect;
}
