#include "game/mechanisms/laser.h"

// game
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/level/fixturenode.h"
#include "game/player/player.h"

#include <iostream>

namespace
{
constexpr std::pair<int32_t, int32_t> range_disabled{0, 1};
constexpr std::pair<int32_t, int32_t> range_enabling{2, 9};
constexpr std::pair<int32_t, int32_t> range_enabled{10, 16};
constexpr std::pair<int32_t, int32_t> range_disabling{17, 20};

constexpr auto range_diabled_delta = range_disabled.second - range_disabled.first;
constexpr auto range_enabled_delta = range_enabled.second - range_enabled.first;

std::vector<std::shared_ptr<TmxObject>> __objects;
std::vector<std::shared_ptr<Laser>> __lasers;
std::vector<std::array<int32_t, 9>> __tiles_version_1;
std::vector<std::array<int32_t, 9>> __tiles_version_2;
}  // namespace

Laser::Laser(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Laser).name());
}

void Laser::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   _sprite->setTextureRect(
      sf::IntRect({_tu * PIXELS_PER_TILE + _tile_index * PIXELS_PER_TILE, _tv * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE})
   );

   color.draw(*_sprite);
}

void Laser::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
}

const sf::FloatRect& Laser::getPixelRect() const
{
   return _pixel_rect;
}

void Laser::update(const sf::Time& dt)
{
   _time += dt.asMilliseconds();

   if (_enabled)
   {
      if (_signal_plot.empty())
      {
         _on = true;
      }
      else
      {
         const auto& sig = _signal_plot.at(_signal_index);

         // elapsed time exceeded signal duration
         if (_time > sig._duration_ms)
         {
            _on = !_on;
            _time = 0;

            // reset signal index after 1 loop
            _signal_index++;
            if (_signal_index >= _signal_plot.size())
            {
               _signal_index = 0;
            }
         }
      }
   }
   else
   {
      _on = false;
   }

   if (_version == MechanismVersion::Version1)
   {
      // shift tile index in right direction depending on the on/off state
      //
      // if the laser is switched on, move the tile index to the left
      // if the laser is switched off, move the tile index to the right
      if ((_on && _tile_index > 0) || (!_on && _tile_index < 6))
      {
         // off sprite is rightmost, on sprite is leftmost
         auto dir = _on ? -1 : 1;

         _tile_animation += (dt.asSeconds() * 10.0f * dir);
         _tile_index = static_cast<int32_t>(_tile_animation);

         // clamp tile index
         if (_tile_index < 0)
         {
            _tile_index = 0;
         }
         else if (_tile_index > 6)
         {
            _tile_index = 6;
         }
      }
   }
   else if (_version == MechanismVersion::Version2)
   {
      //   +---------+-----------+
      //   | frame   | state     |
      //   +---------+-----------+
      //   |  0 -  1 | disabled  |
      //   |  2 -  9 | enabling  |
      //   | 10 - 16 | enabled   |
      //   | 17 - 21 | disabling |
      //   +---------+-----------+

      // disabled (!_on and _tile_index inside 0..1)
      // loop 0..1
      if (!_on && _tile_index >= range_disabled.first && _tile_index <= range_disabled.second)
      {
         _tile_animation += dt.asSeconds();
         _tile_index = range_disabled.first + static_cast<int32_t>(_tile_animation + _animation_offset) % (range_diabled_delta + 1);
      }

      // enabled (_on and _tile_index inside 10..16)
      // loop 10..16
      else if (_on && _tile_index >= range_enabled.first && _tile_index <= range_enabled.second)
      {
         _tile_animation += dt.asSeconds() * 10.0f;
         _tile_index = range_enabled.first + static_cast<int32_t>(_tile_animation + _animation_offset) % (range_enabled_delta + 1);
      }

      // enabling (_on and _tile_index outside 10..16)
      // go from 2..9, when 10 go to range_enabled
      else if (_on)
      {
         _tile_animation += dt.asSeconds() * 10.0f;

         if (_tile_index < range_enabling.first || _tile_index > range_enabling.second)
         {
            _tile_animation = 0;
         }

         _tile_index = range_enabling.first + static_cast<int32_t>(_tile_animation);
      }

      // disabling (!_on and _tile_index outside 0..1)
      // go from 17..21, when 22 to to range_disabled
      else
      {
         _tile_animation += dt.asSeconds() * 10.0f;

         if (_tile_index < range_disabling.first || _tile_index > range_disabling.second)
         {
            _tile_animation = 0;
         }

         _tile_index = range_disabling.first + static_cast<int32_t>(_tile_animation);

         // jumped out of range
         if (_tile_index == range_disabling.second + 1)
         {
            _tile_index = range_disabled.first;
         }
      }
   }

   // move laser
   if (_on)
   {
      if (_path.has_value())
      {
         _path_interpolation.updateTime(_settings._movement_speed * dt.asSeconds());
         _move_offset_px = _path_interpolation.computePosition(_path_interpolation.getTime());
         _sprite->setPosition(_position_px + _move_offset_px);
      }
   }
}

std::optional<sf::FloatRect> Laser::getBoundingBoxPx()
{
   return _pixel_rect;
}

void Laser::reset()
{
   _on = true;
   _tile_index = 0;
   _tile_animation = 0.0f;
   _signal_index = 0;
   _time = 0u;
}

void Laser::resetAll()
{
   __objects.clear();
   __lasers.clear();
   __tiles_version_1.clear();
   __tiles_version_2.clear();
}

const sf::Vector2f& Laser::getTilePosition() const
{
   return _tile_position;
}

const sf::Vector2f& Laser::getPixelPosition() const
{
   return _position_px;
}

std::vector<std::shared_ptr<GameMechanism>> Laser::load(GameNode* parent, const GameDeserializeData& data)
{
   const auto version = (data._tmx_layer->_name == "lasers") ? MechanismVersion::Version1 : MechanismVersion::Version2;

   resetAll();

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

   if (version == MechanismVersion::Version1)
   {
      addTilesVersion1();
   }
   else if (version == MechanismVersion::Version2)
   {
      addTilesVersion2();
   }

   std::vector<std::shared_ptr<GameMechanism>> lasers;

   sf::Vector2u tilesize = sf::Vector2u(data._tmx_tileset->_tile_width_px, data._tmx_tileset->_tile_height_px);
   const auto tiles = data._tmx_layer->_data;
   const auto width = data._tmx_layer->_width_tl;
   const auto height = data._tmx_layer->_height_tl;
   const auto first_id = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         const auto tile_number = tiles[i + j * width];

         if (tile_number == 0)
         {
            continue;
         }

         auto laser = std::make_shared<Laser>(parent);
         lasers.push_back(laser);

         laser->_version = version;

         laser->_tile_position.x = static_cast<float>(i);
         laser->_tile_position.y = static_cast<float>(j);

         laser->_position_px.x = laser->_tile_position.x * PIXELS_PER_TILE;
         laser->_position_px.y = laser->_tile_position.y * PIXELS_PER_TILE;

         laser->_pixel_rect.position.x = laser->_position_px.x;
         laser->_pixel_rect.position.y = laser->_position_px.y;

         laser->_pixel_rect.size.x = PIXELS_PER_TILE;
         laser->_pixel_rect.size.y = PIXELS_PER_TILE;

         laser->_texture = TexturePool::getInstance().get(data._base_path / data._tmx_tileset->_image->_source);

         laser->_tu = (tile_number - first_id) % (laser->_texture->getSize().x / tilesize.x);
         laser->_tv = (tile_number - first_id) / (laser->_texture->getSize().x / tilesize.x);

         if (version == MechanismVersion::Version2)
         {
            laser->_tu = 0;
         }

         if (data._tmx_layer->_properties)
         {
            laser->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
         }

         laser->_sprite = std::make_unique<sf::Sprite>(*laser->_texture);
         laser->_sprite->setPosition(laser->_position_px);

         __lasers.push_back(laser);
      }
   }

   return lasers;
}

void Laser::addObject(const std::shared_ptr<TmxObject>& object)
{
   __objects.push_back(object);
}

void Laser::addTilesVersion1()
{
   // each tile is split up into 3 rows x 3 columns (3 x 8 pixels)
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 1, 0, 0, 0});

   __tiles_version_1.push_back({0, 1, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 1, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 1, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 1, 0, 0, 0});
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 1, 0, 0, 0});
   __tiles_version_1.push_back({0, 1, 0, 1, 1, 1, 0, 1, 0});
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 1, 0, 1, 0});
   __tiles_version_1.push_back({0, 1, 0, 1, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 1, 0, 1, 1, 1, 0, 0, 0});
   __tiles_version_1.push_back({0, 1, 0, 0, 1, 1, 0, 1, 0});
   __tiles_version_1.push_back({0, 0, 0, 0, 1, 1, 0, 1, 0});
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 1, 0, 0, 1, 1, 0, 0, 0});
   __tiles_version_1.push_back({0, 1, 0, 1, 1, 0, 0, 0, 0});
   __tiles_version_1.push_back({0, 1, 0, 0, 1, 0, 0, 0, 0});
   __tiles_version_1.push_back({0, 0, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_1.push_back({0, 0, 0, 0, 1, 1, 0, 0, 0});
   __tiles_version_1.push_back({0, 0, 0, 1, 1, 0, 0, 0, 0});
}

void Laser::addTilesVersion2()
{
   __tiles_version_2.push_back({0, 1, 0, 0, 1, 0, 0, 0, 0});
   __tiles_version_2.push_back({0, 0, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_2.push_back({0, 0, 0, 0, 1, 1, 0, 0, 0});
   __tiles_version_2.push_back({0, 0, 0, 1, 1, 0, 0, 0, 0});
   __tiles_version_2.push_back({0, 1, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_2.push_back({0, 0, 0, 1, 1, 1, 0, 0, 0});
   __tiles_version_2.push_back({0, 0, 0, 0, 1, 1, 0, 1, 0});
   __tiles_version_2.push_back({0, 0, 0, 1, 1, 0, 0, 1, 0});
   __tiles_version_2.push_back({0, 1, 0, 0, 1, 1, 0, 0, 0});
   __tiles_version_2.push_back({0, 1, 0, 1, 1, 0, 0, 0, 0});
   __tiles_version_2.push_back({0, 1, 0, 0, 1, 0, 0, 0, 0});
   __tiles_version_2.push_back({0, 0, 0, 0, 1, 0, 0, 1, 0});
   __tiles_version_2.push_back({0, 0, 0, 0, 1, 1, 0, 0, 0});
   __tiles_version_2.push_back({0, 0, 0, 1, 1, 0, 0, 0, 0});
}

void Laser::collide(const sf::FloatRect& player_rect)
{
   const auto it = std::find_if(
      std::begin(__lasers),
      std::end(__lasers),
      [player_rect](auto laser)
      {
         auto pixel_rect = laser->_pixel_rect;

         if (laser->_path.has_value())
         {
            pixel_rect.position.x += static_cast<int32_t>(laser->_move_offset_px.x);
            pixel_rect.position.y += static_cast<int32_t>(laser->_move_offset_px.y);
         }

         const auto rough_intersection = player_rect.findIntersection(pixel_rect).has_value();

         auto active = false;

         if (laser->_version == MechanismVersion::Version1)
         {
            active = (laser->_tile_index == 0);
         }
         else if (laser->_version == MechanismVersion::Version2)
         {
            active = (laser->_tile_index >= range_enabled.first) && (laser->_tile_index <= range_enabled.second);
         }

         // tile index at 0 is an active laser
         if (active && rough_intersection)
         {
            const auto tile_id = static_cast<uint32_t>(laser->_tv);

            const auto tile = (laser->_version == MechanismVersion::Version1) ? __tiles_version_1[tile_id] : __tiles_version_2[tile_id];

            auto x = 0u;
            auto y = 0u;

            for (auto i = 0u; i < 9; i++)
            {
               if (tile[i] == 1)
               {
                  sf::FloatRect rect;

                  rect.position.x = laser->_position_px.x + (x * PIXELS_PER_PHYSICS_TILE);
                  rect.position.y = laser->_position_px.y + (y * PIXELS_PER_PHYSICS_TILE);

                  if (laser->_path.has_value())
                  {
                     rect.position.x += laser->_move_offset_px.x;
                     rect.position.y += laser->_move_offset_px.y;
                  }

                  rect.size.x = PIXELS_PER_PHYSICS_TILE;
                  rect.size.y = PIXELS_PER_PHYSICS_TILE;

                  const auto fine_intersection = player_rect.findIntersection(rect).has_value();

                  if (fine_intersection)
                  {
                     return true;
                  }
               }

               x++;

               if (i > 0 && ((i + 1) % 3 == 0))
               {
                  x = 0;
                  y++;
               }
            }

            return false;
         }

         return false;
      }
   );

   if (it != __lasers.end())
   {
      // player is dead
      Player::getCurrent()->kill(DeathReason::Laser);
   }
}

void Laser::merge()
{
   int32_t group_id = 0;

   std::vector<std::shared_ptr<TmxObject>> laser_movement_paths;
   std::map<std::string, std::vector<std::shared_ptr<Laser>>> laser_groups;

   for (auto object : __objects)
   {
      // either we have a polygon or polyline that defines the movement path of a
      // laser group or we have a rectangle that defines a new group of lasers
      if (object->_polygon || object->_polyline)
      {
         laser_movement_paths.push_back(object);
      }
      else
      {
         const auto x = static_cast<int32_t>(object->_x_px / PIXELS_PER_TILE);
         const auto y = static_cast<int32_t>(object->_y_px / PIXELS_PER_TILE);
         const auto w = static_cast<int32_t>(object->_width_px / PIXELS_PER_TILE);
         const auto h = static_cast<int32_t>(object->_height_px / PIXELS_PER_TILE);

         const auto animation_offset = std::rand() % 100;

         group_id++;

         std::optional<Signal> on_signal;
         std::optional<Signal> off_signal;
         std::optional<bool> enabled;

         if (object->_properties)
         {
            const auto it_on = object->_properties->_map.find("on_time");
            if (it_on != object->_properties->_map.end())
            {
               on_signal = Signal{static_cast<uint32_t>(it_on->second->_value_int.value()), true};
            }

            const auto it_off = object->_properties->_map.find("off_time");
            if (it_off != object->_properties->_map.end())
            {
               off_signal = Signal{static_cast<uint32_t>(it_off->second->_value_int.value()), false};
            }

            const auto it_enabled = object->_properties->_map.find("enabled");
            if (it_enabled != object->_properties->_map.end())
            {
               enabled = it_enabled->second->_value_bool.value();
            }
         }

         for (auto yi = y; yi < y + h; yi++)
         {
            for (auto xi = x; xi < x + w; xi++)
            {
               for (auto& laser : __lasers)
               {
                  if (static_cast<int32_t>(laser->_tile_position.x) == xi && static_cast<int32_t>(laser->_tile_position.y) == yi)
                  {
                     if (on_signal.has_value())
                     {
                        laser->_signal_plot.push_back(*on_signal);
                     }

                     if (off_signal.has_value())
                     {
                        laser->_signal_plot.push_back(*off_signal);
                     }

                     if (enabled.has_value())
                     {
                        laser->setEnabled(*enabled);
                     }

                     laser->setObjectId(object->_name);
                     laser->_animation_offset = animation_offset;
                     laser->_group_id = group_id;

                     // store laser for being merged later with movement path data
                     laser_groups[object->_name].push_back(laser);
                  }
               }
            }
         }
      }
   }

   // support moving lasers
   // for those merge in the path data retrieved from a polyline or polygon
   for (const auto& tmx_object : laser_movement_paths)
   {
      if (!tmx_object->_properties)
      {
         continue;
      }

      std::optional<std::string> reference_id;
      auto it = tmx_object->_properties->_map.find("reference_id");
      if (it != tmx_object->_properties->_map.end())
      {
         reference_id = it->second->_value_string.value();
      }
      else
      {
         continue;
      }

      std::optional<float> movement_speed;
      const auto movement_speed_it = tmx_object->_properties->_map.find("movement_speed");
      if (movement_speed_it != tmx_object->_properties->_map.end())
      {
         movement_speed = movement_speed_it->second->_value_float.value();
      }

      std::optional<float> move_offset_s;
      const auto it_move_offset_time = tmx_object->_properties->_map.find("move_offset_s");
      if (it_move_offset_time != tmx_object->_properties->_map.end())
      {
         move_offset_s = it_move_offset_time->second->_value_float.value();
      }

      std::optional<std::string> easing_function_name;
      const auto it_easing_function_name = tmx_object->_properties->_map.find("easing_function");
      if (it_easing_function_name != tmx_object->_properties->_map.end())
      {
         easing_function_name = it_easing_function_name->second->_value_string.value();
      }

      std::optional<int32_t> easing_subdivision_count;
      const auto it_easing_subdivision_count = tmx_object->_properties->_map.find("easing_subdivision_count");
      if (it_easing_subdivision_count != tmx_object->_properties->_map.end())
      {
         easing_subdivision_count = it_easing_subdivision_count->second->_value_int.value();
      }

      // fetch path from object and close it
      auto path = tmx_object->_polygon ? tmx_object->_polygon->_polyline : tmx_object->_polyline->_polyline;
      path.push_back(path.at(0));

      const auto& lasers = laser_groups[*reference_id];
      for (const auto& laser : lasers)
      {
         laser->_path = path;

         if (easing_function_name.has_value())
         {
            const auto subdivision_count = easing_function_name.has_value() ? easing_subdivision_count.value() : 10;
            const auto easings_type = Easings::getEnumFromName<float>(easing_function_name.value());
            laser->_path_interpolation.addKeys(path, subdivision_count, easings_type);
         }
         else
         {
            laser->_path_interpolation.addKeys(path);
         }

         if (movement_speed.has_value())
         {
            laser->_settings._movement_speed = *movement_speed;
         }

         if (move_offset_s.has_value())
         {
            laser->_path_interpolation.setTime(*move_offset_s);
         }
      }
   }
}
