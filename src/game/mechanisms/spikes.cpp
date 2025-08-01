#include "spikes.h"

#include "constants.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"

#include <iostream>

// -> 24 - 2 * 4 = 16px rect

//    #  #  #  #  #  #  #  #  #  #  #  #  #  #  #
//    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14
//     | |         |  |                          |
//     | +---------+  +--------------------------+
//     | < extract >  <          retract         >
//     |
//     + unused
constexpr auto SPRITES_PER_CYCLE = 15;
constexpr auto TOLERANCE_PIXELS = 5;
constexpr auto SPIKES_TILE_INDEX_TRAP_START = (SPRITES_PER_CYCLE - 4);
constexpr auto SPIKES_TILE_INDEX_EXTRACT_START = 1;
constexpr auto SPIKES_TILE_INDEX_EXTRACT_END = 4;
constexpr auto SPIKES_TILE_INDEX_RETRACT_START = 6;
constexpr auto SPIKES_TILE_INDEX_RETRACT_END = SPRITES_PER_CYCLE - 1;

namespace
{
auto instance_counter = 0;
}

Spikes::Spikes(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Spikes).name());
   _instance_id = instance_counter++;
}

std::string_view Spikes::objectName() const
{
   return "Spikes";
}

void Spikes::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (const auto& sprite : _sprite)
   {
      color.draw(*sprite);
   }
}

void Spikes::updateInterval()
{
   const auto tu_index = static_cast<int32_t>(std::floor(_tu));

   if (tu_index == SPIKES_TILE_INDEX_EXTRACT_END && _extracting)
   {
      // previously extracting, now start retracting
      _extracting = false;
      _elapsed_ms = 0;
      _idle_time_ms = _config._up_time_ms;
   }

   if (tu_index == SPIKES_TILE_INDEX_RETRACT_END && !_extracting)
   {
      // previously retracting, now start extracting
      _extracting = true;
      _elapsed_ms = 0;
      _idle_time_ms = _config._down_time_ms;
   }

   if (_idle_time_ms.has_value())
   {
      if (_elapsed_ms < _idle_time_ms.value())
      {
         // idle
         return;
      }

      _idle_time_ms.reset();

      // when starting to retract, jump to the corresponding sprite
      if (tu_index == SPIKES_TILE_INDEX_EXTRACT_END)
      {
         _tu = SPIKES_TILE_INDEX_RETRACT_START;
      }

      // when starting to extract, jump to the corresponding sprite
      if (tu_index == SPIKES_TILE_INDEX_RETRACT_END)
      {
         _tu = SPIKES_TILE_INDEX_EXTRACT_START;
      }
   }

   // regular update
   if (_extracting)
   {
      _tu += _config._speed_up * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_EXTRACT_END));
   }
   else
   {
      _tu += _config._speed_down * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_RETRACT_END));
   }
}

void Spikes::updateTrap()
{
   // if already activated, start counting time
   if (_elapsed_since_collision_ms.has_value())
   {
      *_elapsed_since_collision_ms += _dt_ms;
   }
   else
   {
      // trap trigger is done via intersection
      const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
      if (player_rect.findIntersection(_player_collision_rect_px).has_value())
      {
         // start extracting once player has intersected
         _elapsed_since_collision_ms = 0;
      }
   }

   if (_elapsed_since_collision_ms.has_value())
   {
      // check if spikes should be extracting
      const auto extracting = _elapsed_since_collision_ms > _config._trap_time_after_collision_ms;
      if (extracting && !_extracting)
      {
         // when extracting starts, use the first extract sprite
         _extracting = extracting;
         _tu = static_cast<float>(SPIKES_TILE_INDEX_EXTRACT_START);
      }

      if (_extracting)
      {
         _tu += _config._speed_up * _dt_s;
         _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_EXTRACT_END));
      }

      // check if the spikes should move down again
      const auto start_retract = _elapsed_since_collision_ms.value() - _config._trap_time_after_collision_ms > _config._up_time_ms;
      if (start_retract)
      {
         _elapsed_since_collision_ms.reset();
         _extracting = false;
         _tu = static_cast<float>(SPIKES_TILE_INDEX_RETRACT_START);
      }
   }
   else
   {
      // always retract to trap start position if there's no intersection
      _tu += _config._speed_down * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_TRAP_START));
   }
}

void Spikes::updateToggled()
{
   if (isEnabled())
   {
      _tu += _config._speed_up * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_EXTRACT_END));
   }
   else
   {
      _tu += _config._speed_down * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_RETRACT_END));
   }
}

Spikes::Mode Spikes::getMode() const
{
   return _mode;
}

void Spikes::setMode(Mode mode)
{
   _mode = mode;
}

const sf::FloatRect& Spikes::getPixelRect() const
{
   return _player_collision_rect_px;
}

void Spikes::updateSpriteRect()
{
   const auto tu = static_cast<int32_t>(std::floor(_tu));
   for (auto& sprite : _sprite)
   {
      sprite->setTextureRect({{(tu * PIXELS_PER_TILE) + _tu_offset, _tv * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
   }
}

void Spikes::updateDeadly()
{
   _deadly = (_tu < SPRITES_PER_CYCLE - 5);
}

void Spikes::update(const sf::Time& dt)
{
   _elapsed_ms += dt.asMilliseconds();
   _dt_s = dt.asSeconds();
   _dt_ms = dt.asMilliseconds();

   switch (_mode)
   {
      case Mode::Trap:
      {
         updateTrap();
         break;
      }
      case Mode::Interval:
      {
         updateInterval();
         break;
      }
      case Mode::Toggled:
      {
         updateToggled();
         break;
      }
      case Mode::Invalid:
      {
         break;
      }
   }

   updateDeadly();
   updateSpriteRect();

   if (_deadly)
   {
      // check for intersection with player
      const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
      if (player_rect.findIntersection(_player_collision_rect_px).has_value())
      {
         Player::getCurrent()->damage(100);
      }
   }
}

std::optional<sf::FloatRect> Spikes::getBoundingBoxPx()
{
   return _player_collision_rect_px;
}

void Spikes::setEnabled(bool enabled)
{
   if (_mode == Spikes::Mode::Toggled)
   {
      _tu = static_cast<float>(enabled ? SPIKES_TILE_INDEX_EXTRACT_START : SPIKES_TILE_INDEX_RETRACT_START);
   }

   GameMechanism::setEnabled(enabled);
}

std::shared_ptr<Spikes> Spikes::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto instance = std::make_shared<Spikes>(parent);

   instance->setObjectId(data._tmx_object->_name);
   instance->_pixel_position.x = data._tmx_object->_x_px;
   instance->_pixel_position.y = data._tmx_object->_y_px;

   // make the collision rectangle a bit smaller so it's a little more lax
   instance->_player_collision_rect_px = {
      {data._tmx_object->_x_px + TOLERANCE_PIXELS, data._tmx_object->_y_px + TOLERANCE_PIXELS},
      {data._tmx_object->_width_px - (2 * TOLERANCE_PIXELS), data._tmx_object->_height_px - (2 * TOLERANCE_PIXELS)}
   };

   const auto rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   instance->addChunks(rect);

   // deserialize range data
   if (data._tmx_object->_properties)
   {
      // read mode
      const auto mode_it = data._tmx_object->_properties->_map.find("mode");
      if (mode_it != data._tmx_object->_properties->_map.cend())
      {
         const auto mode = mode_it->second->_value_string.value();

         if (mode == "trap")
         {
            instance->_mode = Mode::Trap;
         }
         else if (mode == "interval")
         {
            instance->_mode = Mode::Interval;
         }
         else if (mode == "toggled")
         {
            instance->_mode = Mode::Toggled;
         }
      }

      // read orientation
      const auto orientation_it = data._tmx_object->_properties->_map.find("orientation");
      if (orientation_it != data._tmx_object->_properties->_map.cend())
      {
         const auto orientation = orientation_it->second->_value_string.value();

         if (orientation == "up")
         {
            instance->_orientation = Orientation::PointsUp;
            instance->_tv = 0;
         }
         else if (orientation == "down")
         {
            instance->_orientation = Orientation::PointsDown;
            instance->_tv = 1;
         }
         else if (orientation == "right")
         {
            instance->_orientation = Orientation::PointsRight;
            instance->_tv = 2;
         }
         else if (orientation == "left")
         {
            instance->_orientation = Orientation::PointsLeft;
            instance->_tv = 3;
         }
      }

      auto readIntProperty = [data](int& value, const std::string& id)
      {
         const auto it = data._tmx_object->_properties->_map.find(id);
         if (it != data._tmx_object->_properties->_map.end())
         {
            value = it->second->_value_int.value();
         }
      };

      auto readFloatProperty = [data](float& value, const std::string& id)
      {
         const auto it = data._tmx_object->_properties->_map.find(id);
         if (it != data._tmx_object->_properties->_map.end())
         {
            value = it->second->_value_float.value();
         }
      };

      readIntProperty(instance->_config._down_time_ms, "down_time_ms");
      readIntProperty(instance->_config._up_time_ms, "up_time_ms");
      readIntProperty(instance->_config._trap_time_after_collision_ms, "trap_time_ms");
      readFloatProperty(instance->_config._speed_up, "speed_up");
      readFloatProperty(instance->_config._speed_down, "speed_down");
      readIntProperty(instance->_elapsed_ms, "time_offset_ms");

      const auto under_water_it = data._tmx_object->_properties->_map.find("under_water");
      if (under_water_it != data._tmx_object->_properties->_map.end())
      {
         if (under_water_it->second->_value_bool.value())
         {
            instance->_tu_offset = 15 * PIXELS_PER_TILE;
         }
      }

      auto sprite_count = 0;

      auto x_increment_px = 0;
      auto y_increment_px = 0;
      switch (instance->_orientation)
      {
         case Spikes::Orientation::PointsUp:
         case Spikes::Orientation::PointsDown:
            x_increment_px = PIXELS_PER_TILE;
            sprite_count = static_cast<int32_t>(data._tmx_object->_width_px) / PIXELS_PER_TILE;
            break;
         case Spikes::Orientation::PointsLeft:
         case Spikes::Orientation::PointsRight:
            y_increment_px = PIXELS_PER_TILE;
            sprite_count = static_cast<int32_t>(data._tmx_object->_height_px) / PIXELS_PER_TILE;
            break;
         case Spikes::Orientation::Invalid:
            break;
      }

      auto texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "spikes.png");
      for (auto i = 0; i < sprite_count; i++)
      {
         auto sprite = std::make_unique<sf::Sprite>(*texture);
         sprite->setPosition(sf::Vector2f(
            data._tmx_object->_x_px + static_cast<float>(i * x_increment_px),
            data._tmx_object->_y_px + static_cast<float>(i * y_increment_px)
         ));

         instance->_sprite.push_back(std::move(sprite));
      }
   }

   return instance;
}

std::vector<std::shared_ptr<Spikes>> Spikes::load(GameNode* parent, const GameDeserializeData& data, Mode mode)
{
   if (!data._tmx_layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return {};
   }

   if (!data._tmx_tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return {};
   }

   auto texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "spikes.png");

   std::vector<std::shared_ptr<Spikes>> all_spikes;

   const auto tiles = data._tmx_layer->_data;
   const auto width = data._tmx_layer->_width_tl;
   const auto height = data._tmx_layer->_height_tl;
   const auto first_id = data._tmx_tileset->_first_gid;

   const int32_t tiles_per_row = texture->getSize().x / PIXELS_PER_TILE;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         const auto tile_number = tiles[i + j * width];

         if (tile_number == 0)
         {
            continue;
         }

         const auto id = (tile_number - first_id);

         auto spikes = std::make_shared<Spikes>(parent);
         spikes->_texture = texture;
         spikes->_tu = static_cast<float>(id % tiles_per_row);
         spikes->_tv = static_cast<int32_t>(id / tiles_per_row);
         spikes->_mode = mode;

         all_spikes.push_back(spikes);

         if (mode == Mode::Trap)
         {
            spikes->_tu = SPIKES_TILE_INDEX_TRAP_START;
         }

         if (data._tmx_layer->_properties)
         {
            spikes->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
         }

         spikes->_player_collision_rect_px = {
            {static_cast<float>(i * PIXELS_PER_TILE) + TOLERANCE_PIXELS, static_cast<float>(j * PIXELS_PER_TILE) + TOLERANCE_PIXELS},
            {PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS), PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS)}
         };

         const auto rect = sf::FloatRect{
            {static_cast<float>(i * PIXELS_PER_TILE), static_cast<float>(j * PIXELS_PER_TILE)}, {PIXELS_PER_TILE, PIXELS_PER_TILE}
         };

         std::unique_ptr<sf::Sprite> sprite = std::make_unique<sf::Sprite>(*spikes->_texture);
         sprite->setPosition(sf::Vector2f(static_cast<float>(i * PIXELS_PER_TILE), static_cast<float>(j * PIXELS_PER_TILE)));
         spikes->_sprite.push_back(std::move(sprite));
         spikes->updateSpriteRect();

         spikes->addChunks(rect);
      }
   }

   return all_spikes;
}
