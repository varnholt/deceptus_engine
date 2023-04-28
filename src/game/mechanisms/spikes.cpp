#include "spikes.h"

#include "constants.h"
#include "player/player.h"
#include "texturepool.h"

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"

#include <iostream>

constexpr auto SPRITES_PER_CYCLE = 15;
constexpr auto TOLERANCE_PIXELS = 5;
constexpr auto TRAP_START_TILE = (SPRITES_PER_CYCLE - 4);
constexpr auto SPIKES_TILE_INDEX_FULLY_EXTRACTED = 0;
constexpr auto SPIKES_TILE_INDEX_FULLY_RETRACTED = SPRITES_PER_CYCLE - 1;
constexpr auto SPIKES_TILE_INDEX_MOVE_DOWN_START = 5;
// -> 24 - 2 * 4 = 16px rect

namespace
{
auto instance_counter = 0;
}

Spikes::Spikes(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Spikes).name());
   _instance_id = instance_counter++;

   std::cout << "add instance " << _instance_id << std::endl;
}

void Spikes::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (const auto& sprite : _sprite)
   {
      color.draw(sprite);
   }
}

// 289, 104

namespace
{
auto interval_speed_up = 35.0f;
auto interval_speed_down = 35.0f;
auto trigger_speed_up = 35.0f;
auto trigger_speed_down = 35.0f;
}  // namespace

void Spikes::updateInterval()
{
   const auto tu_index = static_cast<int32_t>(std::floor(_tu));

   if (tu_index == SPIKES_TILE_INDEX_FULLY_EXTRACTED)
   {
      if (_extracting)
      {
         _extracting = false;
         _elapsed_ms = 0;
         _idle_time_ms = _config._up_time_ms;
      }
   }

   if (tu_index == SPIKES_TILE_INDEX_FULLY_RETRACTED)
   {
      if (!_extracting)
      {
         _extracting = true;
         _elapsed_ms = 0;
         _idle_time_ms = _config._down_time_ms;
      }
   }

   if (_idle_time_ms.has_value())
   {
      if (_elapsed_ms < _idle_time_ms.value())
      {
         // idle
         return;
      }

      _idle_time_ms.reset();

      if (_instance_id == 0)
      {
         std::cout << std::endl << std::endl;
      }

      // when starting to retract, jump to the corresponding sprite
      if (tu_index == SPIKES_TILE_INDEX_FULLY_EXTRACTED)
      {
         _tu = SPIKES_TILE_INDEX_MOVE_DOWN_START;
      }
   }

   if (_instance_id == 0)
   {
      std::cout << "tu: " << _tu << ", index: " << tu_index << ", elapsed: " << _elapsed_ms << ", extracting: " << _extracting << std::endl;
   }

   // regular update
   if (_extracting)
   {
      _tu -= interval_speed_up * _dt_s;
      _tu = std::max(_tu, static_cast<float>(SPIKES_TILE_INDEX_FULLY_EXTRACTED));
   }
   else
   {
      _tu += interval_speed_down * _dt_s;
      _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_FULLY_RETRACTED));
   }
}

void Spikes::updateTrap()
{
   const auto tu_index = static_cast<int32_t>(std::floor(_tu));

   if (tu_index == SPIKES_TILE_INDEX_FULLY_EXTRACTED)
   {
      _extracting = false;

      if (_elapsed_ms < _config._up_time_ms)
      {
         return;
      }
   }

   // trap trigger is done via intersection
   if (tu_index == TRAP_START_TILE)
   {
      const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
      if (player_rect.intersects(_pixel_rect))
      {
         // start counting from first intersection
         if (!_extracting)
         {
            _elapsed_ms = 0;
         }

         _extracting = true;
      }

      // trap was activated
      if (_extracting)
      {
         if (_elapsed_ms < _config._trap_time_ms)
         {
            return;
         }
      }
      else
      {
         return;
      }
   }

   const auto update_time_ms = (_extracting ? _config._update_time_up_ms : _config._update_time_down_ms);
   if (_elapsed_ms > update_time_ms)
   {
      _elapsed_ms = (_elapsed_ms % update_time_ms);

      if (_extracting)
      {
         _tu -= trigger_speed_up * _dt_s;
         _tu = std::max(_tu, static_cast<float>(SPIKES_TILE_INDEX_FULLY_EXTRACTED));
      }
      else
      {
         _tu += trigger_speed_down * _dt_s;
         _tu = std::min(_tu, static_cast<float>(SPIKES_TILE_INDEX_FULLY_RETRACTED));
      }
   }
}

void Spikes::updateToggled()
{
   auto tu_index = [&]() { return static_cast<int32_t>(std::floor(_tu)); };

   if (isEnabled())
   {
      if (tu_index() > SPIKES_TILE_INDEX_FULLY_EXTRACTED)
      {
         _tu -= 0.1f;
      }
   }
   else
   {
      if (tu_index() < SPRITES_PER_CYCLE)
      {
         _tu += 0.1f;
      }
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
   return _pixel_rect;
}

void Spikes::updateSpriteRect()
{
   const auto tu = static_cast<int32_t>(std::floor(_tu));
   for (auto& sprite : _sprite)
   {
      sprite.setTextureRect({tu * PIXELS_PER_TILE, _tv * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});
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
      if (player_rect.intersects(_pixel_rect))
      {
         Player::getCurrent()->damage(100);
      }
   }
}

std::optional<sf::FloatRect> Spikes::getBoundingBoxPx()
{
   return _pixel_rect;
}

std::shared_ptr<Spikes> Spikes::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto instance = std::make_shared<Spikes>(parent);

   instance->_pixel_position.x = data._tmx_object->_x_px;
   instance->_pixel_position.y = data._tmx_object->_y_px;
   instance->setObjectId(data._tmx_object->_name);
   instance->_pixel_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

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
         }
         else if (orientation == "down")
         {
            instance->_orientation = Orientation::PointsDown;
         }
         else if (orientation == "right")
         {
            instance->_orientation = Orientation::PointsRight;
         }
         else if (orientation == "left")
         {
            instance->_orientation = Orientation::PointsLeft;
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

      readIntProperty(instance->_config._update_time_up_ms, "update_time_up_ms");
      readIntProperty(instance->_config._update_time_down_ms, "update_time_down_ms");
      readIntProperty(instance->_config._down_time_ms, "down_time_ms");
      readIntProperty(instance->_config._up_time_ms, "up_tims_ms");
      readIntProperty(instance->_config._trap_time_ms, "trap_time_ms");

      const auto under_water_it = data._tmx_object->_properties->_map.find("under_water");
      if (under_water_it != data._tmx_object->_properties->_map.end())
      {
         instance->_under_water = under_water_it->second->_value_bool.value();
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
         auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            const auto id = (tile_number - first_id);

            auto spikes = std::make_shared<Spikes>(parent);
            spikes->_texture = texture;
            spikes->_tu = static_cast<float>(id % tiles_per_row);
            spikes->_tv = static_cast<int32_t>(id / tiles_per_row);
            spikes->_mode = mode;

            all_spikes.push_back(spikes);

            if (mode == Mode::Trap)
            {
               spikes->_tu = TRAP_START_TILE;
            }

            if (data._tmx_layer->_properties)
            {
               spikes->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
            }

            spikes->_pixel_rect = {
               static_cast<float>(i * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               static_cast<float>(j * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS),
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS)};

            sf::Sprite sprite;
            sprite.setTexture(*spikes->_texture);
            sprite.setPosition(sf::Vector2f(static_cast<float>(i * PIXELS_PER_TILE), static_cast<float>(j * PIXELS_PER_TILE)));

            spikes->_sprite.push_back(sprite);
            spikes->updateSpriteRect();
         }
      }
   }

   return all_spikes;
}
