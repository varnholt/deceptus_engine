#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct TmxLayer;
struct TmxTileSet;


struct Physics
{
   void parse(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath
   );

   void parseCollidingTiles(
      TmxLayer* layer,
      TmxTileSet* tileSet
   );

   uint32_t mGridWidth = 0;
   uint32_t mGridHeight = 0;
   uint32_t mGridSize = 0;

   std::vector<int32_t> mPhysicsMap;
};

