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

   bool dumpObj(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& path
   );

   uint32_t _grid_width = 0;
   uint32_t _grid_height = 0;
   uint32_t _grid_size = 0;

   std::vector<int32_t> _physics_map;
};

