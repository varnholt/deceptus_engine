#include "extra.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/audio.h"
#include "game/constants.h"
#include "game/debugdraw.h"
#include "game/extratable.h"
#include "game/gamedeserializedata.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/texturepool.h"

#include <iostream>

// #define DRAW_DEBUG 1

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

   // std::cout << "extra at: " << pos_x_px << ", " << pos_y_px << " (width: " << width_px << ", height: " << height_px << ")" << std::endl;

   _name = data._tmx_object->_name;
   _rect = {pos_x_px, pos_y_px, width_px, height_px};

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
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

      // read animations if set up
      const auto offset_x = width_px * 0.5f;
      const auto offset_y = height_px * 0.5f;
      AnimationPool animation_pool{"data/sprites/extra_animations.json"};
      const auto animation_pickup = data._tmx_object->_properties->_map.find("animation_pickup");
      if (animation_pickup != data._tmx_object->_properties->_map.end())
      {
         const auto key = animation_pickup->second->_value_string.value();
         _animation_pickup = animation_pool.create(key, pos_x_px + offset_x, pos_y_px + offset_y, false, false);
      }

      for (auto i = 0; i < 99; i++)
      {
         const auto main_animation_key = "animation_main_" + std::to_string(i);
         const auto main_animation_n = data._tmx_object->_properties->_map.find(main_animation_key);
         if (main_animation_n != data._tmx_object->_properties->_map.end())
         {
            const auto key = main_animation_n->second->_value_string.value();
            auto main_animation = animation_pool.create(key, pos_x_px + offset_x, pos_y_px + offset_y, false, false);
            _animations_main.push_back(main_animation);
         }
      }
   }

   if (!_animations_main.empty())
   {
      _animations_main_it = _animations_main.begin();
   }

   // add
   // - enable/disable mechanism function to level
   // - add enable/disable mechanism code to levelscript
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (_animation_pickup && !_animation_pickup->_paused)
   {
      _animation_pickup->draw(target);
   }

   if (!_active)
   {
      return;
   }

   // draw animations
   if (_animations_main_it->get())
   {
      _animations_main_it->get()->draw(target);
   }

   // or show static extra texture
   else
   {
      target.draw(_sprite);
   }

#ifdef DRAW_DEBUG
   DebugDraw::drawRect(target, _rect);
#endif
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::update(const sf::Time& dt)
{
   // play pickup animation if active
   if (_animation_pickup && !_animation_pickup->_paused)
   {
      _animation_pickup->update(dt);
   }

   if (!_active)
   {
      return;
   }

   // update regular animations
   if (*_animations_main_it)
   {
      (*_animations_main_it)->update(dt);
      if ((*_animations_main_it)->_paused)
      {
         _animations_main_it++;

         // start loop again if needed
         if (_animations_main_it == _animations_main.end())
         {
            _animations_main_it = _animations_main.begin();
         }

         (*_animations_main_it)->seekToStart();
         (*_animations_main_it)->play();
      }
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
