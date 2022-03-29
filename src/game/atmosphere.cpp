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
void Atmosphere::parse(TmxLayer* layer, TmxTileSet* tileset)
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
   const auto width = layer->_width_px;
   const auto height = layer->_height_px;
   const auto offset_x = layer->_offset_x_px;
   const auto offset_y = layer->_offset_y_px;

   _map.resize(width * height);
   _map_width = width;
   _map_height = height;
   _map_offset_x_m = offset_x;
   _map_offset_y_m = offset_y;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         const auto tile_number = tiles[y * width + x];
         auto tile_relative = static_cast<int32_t>(AtmosphereTileInvalid);
         if (tile_number != 0)
         {
            tile_relative = tile_number - tileset->_first_gid;
            _map[y * width + x] = tile_relative;
         }

          _map[y * width + x] = tile_relative;
      }
   }
}
