#include "atmosphere.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"

Atmosphere::~Atmosphere()
{
   _map.clear();
}

void Atmosphere::parse(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset)
{
   if (!layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return;
   }

   if (!tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return;
   }

   const auto tiles = layer->_data;
   const auto width_tl = layer->_width_tl;
   const auto height_tl = layer->_height_tl;

   _map.resize(width_tl * height_tl);
   _map_width_tl = width_tl;
   _map_height_tl = height_tl;

   for (auto y_tl = 0u; y_tl < height_tl; y_tl++)
   {
      for (auto x_tl = 0u; x_tl < width_tl; x_tl++)
      {
         // get the current tile number
         const auto tile_number = tiles[y_tl * width_tl + x_tl];
         auto tile_relative = static_cast<int32_t>(AtmosphereTileInvalid);
         if (tile_number != 0)
         {
            tile_relative = tile_number - tileset->_first_gid;
         }

         _map[y_tl * width_tl + x_tl] = tile_relative;
      }
   }
}

bool Atmosphere::hasTileInRect(const sf::FloatRect& rect_px) const
{
   if (_map.empty())
   {
      return false;
   }

   const auto first_x_tl = std::max(0, static_cast<int32_t>(std::floor(rect_px.position.x / PIXELS_PER_TILE)));
   const auto first_y_tl = std::max(0, static_cast<int32_t>(std::floor(rect_px.position.y / PIXELS_PER_TILE)));
   const auto last_x_tl =
      std::min(_map_width_tl - 1, static_cast<int32_t>(std::floor((rect_px.position.x + rect_px.size.x) / PIXELS_PER_TILE)));
   const auto last_y_tl =
      std::min(_map_height_tl - 1, static_cast<int32_t>(std::floor((rect_px.position.y + rect_px.size.y) / PIXELS_PER_TILE)));

   for (auto y_tl = first_y_tl; y_tl <= last_y_tl; y_tl++)
   {
      for (auto x_tl = first_x_tl; x_tl <= last_x_tl; x_tl++)
      {
         if (_map[y_tl * _map_width_tl + x_tl] != static_cast<int32_t>(AtmosphereTileInvalid))
         {
            return true;
         }
      }
   }

   return false;
}

AtmosphereTile Atmosphere::getTileForPosition(const b2Vec2& pos_m) const
{
   const auto x_px = pos_m.x * PPM;
   const auto y_px = pos_m.y * PPM;

   return getTileForPosition(sf::Vector2f{x_px, y_px});
}

AtmosphereTile Atmosphere::getTileForPosition(const sf::Vector2f& pos_px) const
{
   const auto x_tl = static_cast<int32_t>(pos_px.x / PIXELS_PER_TILE);
   const auto y_tl = static_cast<int32_t>(pos_px.y / PIXELS_PER_TILE);

   if (x_tl < 0 || x_tl >= _map_width_tl)
   {
      return AtmosphereTileInvalid;
   }

   if (y_tl < 0 || y_tl >= _map_height_tl)
   {
      return AtmosphereTileInvalid;
   }

   const auto tile = static_cast<AtmosphereTile>(_map[y_tl * _map_width_tl + x_tl]);
   return tile;
}
