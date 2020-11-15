#include "atmosphere.h"

#include <iostream>

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"


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

   auto tiles  = layer->mData;
   auto width  = layer->mWidth;
   auto height = layer->mHeight;
   auto offsetX = layer->mOffsetX;
   auto offsetY = layer->mOffsetY;

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
            tileRelative = tileNumber - tileSet->mFirstGid;
            mMap[y * width + x] = tileRelative;
         }

          mMap[y * width + x] = tileRelative;
      }
   }
}
