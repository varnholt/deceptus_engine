#include "laser.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/log.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "player/player.h"
#include "texturepool.h"

#include <iostream>


namespace
{
static constexpr std::pair<int32_t, int32_t> range_disabled{0, 1};
static constexpr std::pair<int32_t, int32_t> range_enabling{2, 9};
static constexpr std::pair<int32_t, int32_t> range_enabled{10, 16};
static constexpr std::pair<int32_t, int32_t> range_disabling{17, 20};

static constexpr auto range_diabled_delta  = range_disabled.second  - range_disabled.first;
static constexpr auto range_enabled_delta  = range_enabled.second   - range_enabled.first;
}


//-----------------------------------------------------------------------------
std::vector<TmxObject*> Laser::__objects;
std::vector<std::shared_ptr<Laser>> Laser::__lasers;
std::vector<std::array<int32_t, 9>> Laser::__tiles_version_1;
std::vector<std::array<int32_t, 9>> Laser::__tiles_version_2;


//-----------------------------------------------------------------------------
Laser::Laser(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(Laser).name());
}


//-----------------------------------------------------------------------------
void Laser::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   _sprite.setTextureRect(
      sf::IntRect(
         _tu * PIXELS_PER_TILE + _tile_index * PIXELS_PER_TILE,
         _tv * PIXELS_PER_TILE,
         PIXELS_PER_TILE,
         PIXELS_PER_TILE
      )
   );

   color.draw(_sprite);
}



//-----------------------------------------------------------------------------
void Laser::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
}


//-----------------------------------------------------------------------------
const sf::Rect<int32_t>& Laser::getPixelRect() const
{
   return _pixel_rect;
}


//-----------------------------------------------------------------------------
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
         if (_time > sig.mDurationMs)
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
      if ( (_on && _tile_index > 0) || (!_on && _tile_index < 6) )
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
}


//-----------------------------------------------------------------------------
void Laser::reset()
{
   _on = true;
   _tile_index = 0;
   _tile_animation = 0.0f;
   _signal_index = 0;
   _time = 0u;
}


//-----------------------------------------------------------------------------
void Laser::resetAll()
{
   __objects.clear();
   __lasers.clear();
   __tiles_version_1.clear();
   __tiles_version_2.clear();
}


//-----------------------------------------------------------------------------
const sf::Vector2f& Laser::getTilePosition() const
{
   return _tile_position;
}


//-----------------------------------------------------------------------------
const sf::Vector2f& Laser::getPixelPosition() const
{
   return _pixel_position;
}


//-----------------------------------------------------------------------------
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
   const auto tiles    = data._tmx_layer->_data;
   const auto width    = data._tmx_layer->_width_tl;
   const auto height   = data._tmx_layer->_height_tl;
   const auto firstId  = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         int tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto laser = std::make_shared<Laser>(parent);
            lasers.push_back(laser);

            laser->_version = version;

            laser->_tile_position.x = static_cast<float>(i);
            laser->_tile_position.y = static_cast<float>(j);

            laser->_pixel_position.x = laser->_tile_position.x * PIXELS_PER_TILE;
            laser->_pixel_position.y = laser->_tile_position.y * PIXELS_PER_TILE;

            laser->_pixel_rect.left = static_cast<int32_t>(laser->_pixel_position.x);
            laser->_pixel_rect.top  = static_cast<int32_t>(laser->_pixel_position.y);

            laser->_pixel_rect.width  = PIXELS_PER_TILE;
            laser->_pixel_rect.height = PIXELS_PER_TILE;

            laser->_texture = TexturePool::getInstance().get(data._base_path / data._tmx_tileset->_image->_source);

            laser->_tu = (tileNumber - firstId) % (laser->_texture->getSize().x / tilesize.x);
            laser->_tv = (tileNumber - firstId) / (laser->_texture->getSize().x / tilesize.x);

            if (version == MechanismVersion::Version2)
            {
               laser->_tu = 0;
            }

            if (data._tmx_layer->_properties != nullptr)
            {
               laser->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
            }

            sf::Sprite sprite;
            sprite.setTexture(*laser->_texture);
            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            laser->_sprite = sprite;
            __lasers.push_back(laser);
         }
      }
   }

   return lasers;
}


void Laser::addObject(TmxObject* object)
{
   __objects.push_back(object);
}


void Laser::addTilesVersion1()
{
   // each tile is split up into 3 rows x 3 columns (3 x 8 pixels)
   __tiles_version_1.push_back(
      {0,0,0,
       1,1,1,
       0,0,0}
   );

   __tiles_version_1.push_back({0,1,0,0,1,0,0,1,0});
   __tiles_version_1.push_back({0,1,0,0,1,0,0,1,0});
   __tiles_version_1.push_back({0,1,0,0,1,0,0,1,0});
   __tiles_version_1.push_back({0,0,0,1,1,1,0,0,0});
   __tiles_version_1.push_back({0,0,0,1,1,1,0,0,0});
   __tiles_version_1.push_back({0,1,0,1,1,1,0,1,0});
   __tiles_version_1.push_back({0,0,0,1,1,1,0,1,0});
   __tiles_version_1.push_back({0,1,0,1,1,0,0,1,0});
   __tiles_version_1.push_back({0,1,0,1,1,1,0,0,0});
   __tiles_version_1.push_back({0,1,0,0,1,1,0,1,0});
   __tiles_version_1.push_back({0,0,0,0,1,1,0,1,0});
   __tiles_version_1.push_back({0,0,0,1,1,0,0,1,0});
   __tiles_version_1.push_back({0,1,0,0,1,1,0,0,0});
   __tiles_version_1.push_back({0,1,0,1,1,0,0,0,0});
   __tiles_version_1.push_back({0,1,0,0,1,0,0,0,0});
   __tiles_version_1.push_back({0,0,0,0,1,0,0,1,0});
   __tiles_version_1.push_back({0,0,0,0,1,1,0,0,0});
   __tiles_version_1.push_back({0,0,0,1,1,0,0,0,0});
}


void Laser::addTilesVersion2()
{
   __tiles_version_2.push_back({0,1,0,0,1,0,0,0,0});
   __tiles_version_2.push_back({0,0,0,0,1,0,0,1,0});
   __tiles_version_2.push_back({0,0,0,0,1,1,0,0,0});
   __tiles_version_2.push_back({0,0,0,1,1,0,0,0,0});
   __tiles_version_2.push_back({0,1,0,0,1,0,0,1,0});
   __tiles_version_2.push_back({0,0,0,1,1,1,0,0,0});
   __tiles_version_2.push_back({0,0,0,0,1,1,0,1,0});
   __tiles_version_2.push_back({0,0,0,1,1,0,0,1,0});
   __tiles_version_2.push_back({0,1,0,0,1,1,0,0,0});
   __tiles_version_2.push_back({0,1,0,1,1,0,0,0,0});
   __tiles_version_2.push_back({0,1,0,0,1,0,0,0,0});
   __tiles_version_2.push_back({0,0,0,0,1,0,0,1,0});
   __tiles_version_2.push_back({0,0,0,0,1,1,0,0,0});
   __tiles_version_2.push_back({0,0,0,1,1,0,0,0,0});
}


void Laser::collide(const sf::Rect<int32_t>& player_rect)
{
   const auto it =
      std::find_if(std::begin(__lasers), std::end(__lasers), [player_rect](auto laser) {

            const auto roughIntersection = player_rect.intersects(laser->_pixel_rect);

            auto active = false;

            if (laser->_version == MechanismVersion::Version1)
            {
               active = (laser->_tile_index == 0);
            }
            else if (laser->_version == MechanismVersion::Version2)
            {
               active = (laser->_tile_index >= range_enabled.first) && (laser->_tile_index <= range_enabled.second);
            }

            // tileindex at 0 is an active laser
            if (active && roughIntersection)
            {
               const auto tile_id = static_cast<uint32_t>(laser->_tv);

               const auto tile =
                  (laser->_version == MechanismVersion::Version1)
                     ? __tiles_version_1[tile_id]
                     : __tiles_version_2[tile_id];

               auto x = 0u;
               auto y = 0u;

               for (auto i = 0u; i < 9; i++)
               {
                  if (tile[i] == 1)
                  {
                     sf::Rect<int32_t> rect;

                     rect.left = static_cast<int32_t>(laser->_pixel_position.x + (x * PIXELS_PER_PHYSICS_TILE));
                     rect.top  = static_cast<int32_t>(laser->_pixel_position.y + (y * PIXELS_PER_PHYSICS_TILE));

                     rect.width  = PIXELS_PER_PHYSICS_TILE;
                     rect.height = PIXELS_PER_PHYSICS_TILE;

                     const auto fineIntersection = player_rect.intersects(rect);

                     if (fineIntersection)
                     {
                        return true;
                     }
                  }

                  x++;

                  if (i > 0 && ( (i + 1) % 3 == 0))
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
      Player::getCurrent()->damage(100);
   }
}


void Laser::merge()
{
   int32_t group_id = 0;

   for (auto object : __objects)
   {
      const auto x = static_cast<int32_t>(object->_x_px      / PIXELS_PER_TILE );
      const auto y = static_cast<int32_t>(object->_y_px      / PIXELS_PER_TILE);
      const auto w = static_cast<int32_t>(object->_width_px  / PIXELS_PER_TILE );
      const auto h = static_cast<int32_t>(object->_height_px / PIXELS_PER_TILE);

      const auto animation_offset = std::rand() % 100;

      group_id++;

      for (auto yi = y; yi < y + h; yi++)
      {
         for (auto xi = x; xi < x + w; xi++)
         {
            for (auto& laser : __lasers)
            {
               if (
                     static_cast<int32_t>(laser->_tile_position.x) == xi
                  && static_cast<int32_t>(laser->_tile_position.y) == yi
               )
               {
                  if (object->_properties != nullptr)
                  {
                     auto it = object->_properties->_map.find("on_time");
                     if (it != object->_properties->_map.end())
                     {
                         laser->_signal_plot.push_back(Signal{static_cast<uint32_t>(it->second->_value_int.value()), true});
                     }

                     it = object->_properties->_map.find("off_time");
                     if (it != object->_properties->_map.end())
                     {
                         laser->_signal_plot.push_back(Signal{static_cast<uint32_t>(it->second->_value_int.value()), false});
                     }
                  }

                  laser->_animation_offset = animation_offset;
                  laser->_group_id = group_id;
               }
            }
         }
      }
   }
}


