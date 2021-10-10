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
void Atmosphere::parse(TmxLayer* layer, TmxTileSet* tileSet)
{
   if (layer == nullptr)
   {
     Log::Error() << "physics tmx layer is a nullptr";
     exit(-1);
   }

   if (tileSet == nullptr)
   {
     Log::Error() << "physics tmx tileset is a nullptr";
     exit(-1);
   }

   auto tiles  = layer->_data;
   auto width  = layer->_width_px;
   auto height = layer->_height_px;
   auto offsetX = layer->_offset_x_px;
   auto offsetY = layer->_offset_y_px;

   _map.resize(width * height);
   _map_width = width;
   _map_height = height;
   _map_offset_x = offsetX;
   _map_offset_y = offsetY;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tileNumber = tiles[y * width + x];
         auto tileRelative = static_cast<int32_t>(AtmosphereTileInvalid);
         if (tileNumber != 0)
         {
            tileRelative = tileNumber - tileSet->_first_gid;
            _map[y * width + x] = tileRelative;
         }

          _map[y * width + x] = tileRelative;
      }
   }
}
