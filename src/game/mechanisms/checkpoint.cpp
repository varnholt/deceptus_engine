#include "checkpoint.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/log.h"
#include "game/audio/audio.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "game/state/savestate.h"

#include <iostream>

/*

   +----+----+----+
   |    | .. |    |
   +----+----+----+
   |  ::|####|::  |
   +----+####+----+
   |  ::|####|::  |
   +----+####+----+
   |  ::|####|::  |
   +----+----+----+
   |    | .. |    |
   +----+----+----+

   core clock is 24 x 72
   clock canvas is 72 * 120

   activation is 20 sprites per row
   active is 14 sprites per row

   tic is column 3 (left)
   toc is column 9 (right)
*/

namespace
{
constexpr auto tick_index = 3;
constexpr auto tock_index = 9;
}  // namespace

Checkpoint::Checkpoint(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Checkpoint).name());

   Audio::getInstance().addSample("wallclock_tock.wav");
   Audio::getInstance().addSample("wallclock_tick.wav");

   _has_audio = true;
   _audio_update_data._range = AudioRange{12 * PIXELS_PER_TILE, 0.0f, 2 * PIXELS_PER_TILE, 1.0f};
}

std::shared_ptr<Checkpoint> Checkpoint::getCheckpoint(int32_t index, const std::vector<std::shared_ptr<GameMechanism>>& checkpoints)
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
   const auto rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   auto checkpoint = std::make_shared<Checkpoint>(parent);
   checkpoint->setObjectId(data._tmx_object->_name);
   checkpoint->_texture = TexturePool::getInstance().get("data/sprites/checkpoint.png");
   checkpoint->_sprite.setTexture(*checkpoint->_texture);
   checkpoint->_rect = rect;
   checkpoint->_name = data._tmx_object->_name;
   checkpoint->updateSpriteRect();
   checkpoint->addChunks(rect);

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
      auto sprite_pos_x_it = data._tmx_object->_properties->_map.find("sprite_pos_x_px");
      auto sprite_pos_y_it = data._tmx_object->_properties->_map.find("sprite_pos_y_px");
      if (sprite_pos_x_it != data._tmx_object->_properties->_map.end() && sprite_pos_y_it != data._tmx_object->_properties->_map.end())
      {
         sf::Vector2f pos{
            static_cast<float>(sprite_pos_x_it->second->_value_int.value()), static_cast<float>(sprite_pos_y_it->second->_value_int.value())
         };
         checkpoint->_sprite.setPosition(pos);
      }
      else
      {
         checkpoint->_sprite.setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});
      }
   }

   // whenever we reach a checkpoint, update the checkpoint index in the save state
   // and serialize the save state
   const auto cp_index = checkpoint->getIndex();
   checkpoint->addCallback([]() { Level::getCurrentLevel()->saveState(); });
   checkpoint->addCallback([cp_index]() { SaveState::getCurrent()._checkpoint = cp_index; });
   checkpoint->addCallback([]() { SaveState::serializeToFile(); });

   // that y offset is a litte dodgy, could have something cleaner in the future
   constexpr auto player_placement_offset_px = -10;
   checkpoint->_spawn_point =
      sf::Vector2f{rect.left + rect.width / 2, rect.top + rect.height - PIXELS_PER_TILE + player_placement_offset_px};

   return checkpoint;
}

void Checkpoint::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);

   //   DebugDraw::drawRect(target, _rect);
}

void Checkpoint::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();

   if (player_rect.intersects(_rect))
   {
      reached();
   }

   if (_reached)
   {
      updateSpriteRect(dt.asSeconds());
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

   // doesn't make sense to show fancy reveal animation if this is the active checkpoint anyway
   if (SaveState::getCurrent()._checkpoint != _index)
   {
      _state = State::Activating;

      // play reveal sound
      Audio::getInstance().playSample({"player_spawn_01.wav"});
   }
   else
   {
      _state = State::Active;
   }

   for (const auto& callback : _callbacks)
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

sf::Vector2f Checkpoint::spawnPoint() const
{
   return _spawn_point;
}

int32_t Checkpoint::getIndex() const
{
   return _index;
}

void Checkpoint::updateSpriteRect(float dt_s)
{
   constexpr auto animation_speed_activate = 20.0f;
   constexpr auto animation_speed_active = 10.0f;

   int32_t x = 0;
   int32_t y = 0;
   constexpr int32_t w = 3 * PIXELS_PER_TILE;
   constexpr int32_t h = 5 * PIXELS_PER_TILE;
   switch (_state)
   {
      case State::Inactive:
         break;
      case State::Activating:
         _sprite_index += dt_s * animation_speed_activate;
         x = static_cast<int32_t>(std::floor(_sprite_index)) * w;
         if (_sprite_index > 19)
         {
            _state = State::Active;
            _sprite_index = 0.0f;
         }
         break;
      case State::Active:
         _sprite_index += dt_s * animation_speed_active;
         _sprite_index = fmod(_sprite_index, 14.0f);
         x = static_cast<int32_t>(std::floor(_sprite_index)) * w;
         y = h;

         if (static_cast<int32_t>(_sprite_index) == tick_index)
         {
            if (!_tick_played)
            {
               _tick_played = true;
               _tock_played = false;
               Audio::getInstance().playSample({"wallclock_tock.wav", _reference_volume});
            }
         }
         else if (static_cast<int32_t>(_sprite_index) == tock_index)
         {
            if (!_tock_played)
            {
               _tock_played = true;
               _tick_played = false;
               Audio::getInstance().playSample({"wallclock_tick.wav", _reference_volume});
            }
         }

         break;
   }

   _sprite.setTextureRect({x, y, w, h});
}
