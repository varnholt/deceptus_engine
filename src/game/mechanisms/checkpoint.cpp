#include "checkpoint.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/log.h"
#include "level.h"
#include "player/player.h"
#include "savestate.h"
#include "texturepool.h"

#include <iostream>

Checkpoint::Checkpoint(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Checkpoint).name());
}

std::shared_ptr<Checkpoint> Checkpoint::getCheckpoint(uint32_t index, const std::vector<std::shared_ptr<GameMechanism>>& checkpoints)
{
   const auto& it = std::find_if(
      checkpoints.begin(),
      checkpoints.end(),
      [index](const auto& tmp) { return std::dynamic_pointer_cast<Checkpoint>(tmp)->getIndex() == index; }
   );

   if (it != checkpoints.end())
   {
      return std::dynamic_pointer_cast<Checkpoint>(*it);
   }

   return nullptr;
}

std::shared_ptr<Checkpoint> Checkpoint::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto checkpoint = std::make_shared<Checkpoint>(parent);
   checkpoint->setObjectId(data._tmx_object->_name);

   checkpoint->_texture = TexturePool::getInstance().get("data/sprites/checkpoint.png");
   checkpoint->_sprite.setTexture(*checkpoint->_texture);
   checkpoint->updateSpriteRect();

   checkpoint->_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   checkpoint->_name = data._tmx_object->_name;

   if (data._tmx_object->_properties)
   {
      auto it = data._tmx_object->_properties->_map.find("index");
      if (it != data._tmx_object->_properties->_map.end())
      {
         checkpoint->_index = static_cast<int32_t>(it->second->_value_int.value());
      }
      else
      {
         Log::Error() << "checkpoint " << checkpoint->_name << " does not have an index, please fix your level";
      }

      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         auto z_index = static_cast<int32_t>(z_it->second->_value_int.value());
         checkpoint->setZ(z_index);
      }

      // update sprite position
      sf::Vector2f pos;

      auto sprite_pos_x_it = data._tmx_object->_properties->_map.find("sprite_pos_x_px");
      auto sprite_pos_y_it = data._tmx_object->_properties->_map.find("sprite_pos_y_px");

      if (sprite_pos_x_it != data._tmx_object->_properties->_map.end())
      {
         pos.x = static_cast<float>(sprite_pos_x_it->second->_value_int.value());
      }

      if (sprite_pos_y_it != data._tmx_object->_properties->_map.end())
      {
         pos.y = static_cast<float>(sprite_pos_y_it->second->_value_int.value());
      }

      checkpoint->_sprite.setPosition(pos);
   }

   // whenever we reach a checkpoint, update the checkpoint index in the save state
   // and serialize the save state
   const auto cp_index = checkpoint->getIndex();
   checkpoint->addCallback([]() { Level::getCurrentLevel()->saveState(); });
   checkpoint->addCallback([cp_index]() { SaveState::getCurrent()._checkpoint = cp_index; });
   checkpoint->addCallback([]() { SaveState::serializeToFile(); });

   return checkpoint;
}

void Checkpoint::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}

void Checkpoint::update(const sf::Time& /*dt*/)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();

   if (player_rect.intersects(_rect))
   {
      reached();
   }
}

std::optional<sf::FloatRect> Checkpoint::getBoundingBoxPx()
{
   return _rect;
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

sf::Vector2f Checkpoint::calcCenter() const
{
   // that y offset is a litte dodgy, could have something cleaner in the future
   sf::Vector2f pos{_rect.left + _rect.width / 2, _rect.top + _rect.height - 10};
   return pos;
}

int32_t Checkpoint::getIndex() const
{
   return _index;
}

void Checkpoint::updateSpriteRect()
{
   _sprite.setTextureRect({_reached ? 48 : 0, 0, 48, 48});
}
