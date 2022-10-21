#include "atmosphere.h"

#include <iostream>

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"


//-----------------------------------------------------------------------------
Atmosphere::~Atmosphere()
{
   _map.clear();
}


//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
AtmosphereTile Atmosphere::getTileForPosition(const b2Vec2& pos_m) const
{
   const auto x_px = pos_m.x * PPM;
   const auto y_px = pos_m.y * PPM;

   return getTileForPosition(sf::Vector2f{x_px, y_px});
}

//-----------------------------------------------------------------------------
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
