#include "atmosphere.h"

#include <iostream>

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"


//-----------------------------------------------------------------------------
Atmosphere::~Atmosphere()
{
   mMap.clear();
}


//-----------------------------------------------------------------------------
void Atmosphere::parse(TmxLayer* layer, TmxTileSet* tileSet)
{
   if (layer == nullptr)
   {
     std::cout << "physics tmx layer is a nullptr" << std::endl;
     exit(-1);
   }

   if (tileSet == nullptr)
   {
     std::cout << "physics tmx tileset is a nullptr" << std::endl;
     exit(-1);
   }

   auto tiles  = layer->_data;
   auto width  = layer->_width_px;
   auto height = layer->_height_px;
   auto offsetX = layer->_offset_x_px;
   auto offsetY = layer->_offset_y_px;

   mMap.resize(width * height);
   mMapWidth = width;
   mMapHeight = height;
   mMapOffsetX = offsetX;
   mMapOffsetY = offsetY;

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
            mMap[y * width + x] = tileRelative;
         }

          mMap[y * width + x] = tileRelative;
      }
   }
}
