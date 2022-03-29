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
