#include "checkpoint.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/log.h"
#include "player/player.h"
#include "texturepool.h"

#include <iostream>


Checkpoint::Checkpoint(GameNode* parent)
 : GameNode(parent)
{
}


std::shared_ptr<Checkpoint> Checkpoint::getCheckpoint(
   uint32_t index,
   const std::vector<std::shared_ptr<GameMechanism>>& checkpoints
)
{
   const auto& it =
      std::find_if(
         checkpoints.begin(),
         checkpoints.end(),
         [index](const auto& tmp) {
            return std::dynamic_pointer_cast<Checkpoint>(tmp)->getIndex() == index;
         }
      );

   if (it != checkpoints.end())
   {
      return std::dynamic_pointer_cast<Checkpoint>(*it);
   }

   return nullptr;
}


std::shared_ptr<Checkpoint> Checkpoint::deserialize(GameNode* parent, TmxObject* tmx_object)
{
   auto cp = std::make_shared<Checkpoint>(parent);

   cp->_texture = TexturePool::getInstance().get("data/sprites/checkpoint.png");
   cp->_sprite.setTexture(*cp->_texture);
   cp->updateSpriteRect();

   cp->_rect = sf::IntRect{
      static_cast<int32_t>(tmx_object->_x_px),
      static_cast<int32_t>(tmx_object->_y_px),
      static_cast<int32_t>(tmx_object->_width_px),
      static_cast<int32_t>(tmx_object->_height_px)
   };

   cp->_name = tmx_object->_name;

   if (tmx_object->_properties)
   {
      auto it = tmx_object->_properties->_map.find("index");
      if (it != tmx_object->_properties->_map.end())
      {
         cp->_index = static_cast<uint32_t>(it->second->_value_int.value());
      }

      auto z_it = tmx_object->_properties->_map.find("z");
      if (z_it != tmx_object->_properties->_map.end())
      {
         auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         cp->setZ(z_index);
      }

      // update sprite position
      sf::Vector2f pos;

      auto sprite_pos_x_it = tmx_object->_properties->_map.find("sprite_pos_x_px");
      auto sprite_pos_y_it = tmx_object->_properties->_map.find("sprite_pos_y_px");

      if (sprite_pos_x_it != tmx_object->_properties->_map.end())
      {
         pos.x = static_cast<float>(sprite_pos_x_it->second->_value_int.value());
      }

      if (sprite_pos_y_it != tmx_object->_properties->_map.end())
      {
         pos.y = static_cast<float>(sprite_pos_y_it->second->_value_int.value());
      }

      cp->_sprite.setPosition(pos);
   }

   return cp;
}


void Checkpoint::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void Checkpoint::update(const sf::Time& /*dt*/)
{
   const auto& player_rect = Player::getCurrent()->getPlayerPixelRect();

   if (player_rect.intersects(_rect))
   {
      reached();
   }
}


void Checkpoint::reached()
{
   if (_reached)
   {
      return;
   }

   Log::Info() << "reached checkpoint: " << _index;

   _reached = true;
   updateSpriteRect();

   for (auto& callback : _callbacks)
   {
      callback();
   }

   // check if level is completed
   if (_name == "end")
   {
      // CallbackMap::getInstance().call(static_cast<int32_t>(CallbackType::EndGame));
      CallbackMap::getInstance().call(static_cast<int32_t>(CallbackType::NextLevel));
   }
}


void Checkpoint::addCallback(Checkpoint::CheckpointCallback cb)
{
   _callbacks.push_back(cb);
}


sf::Vector2i Checkpoint::calcCenter() const
{
   // that y offset is a litte dodgy, could have something cleaner in the future
   sf::Vector2i pos{_rect.left + _rect.width / 2, _rect.top + _rect.height - 10};
   return pos;
}


uint32_t Checkpoint::getIndex() const
{
   return _index;
}


void Checkpoint::updateSpriteRect()
{
   _sprite.setTextureRect({
         _reached ? 48 : 0,
         0,
         48,
         48
      }
   );
}


