#pragma once

// base
#include "tmxelement.h"

#include <cstdint>
#include <filesystem>
#include <map>

// forward declarations
struct TmxImage;
struct TmxTile;

struct TmxTileSet : TmxElement
{
   TmxTileSet();
   ~TmxTileSet() override;

   void deserialize(tinyxml2::XMLElement* e) override;
   void parseTileSet(tinyxml2::XMLElement* element);

   std::string _source;
   int32_t _first_gid = 0;
   int32_t _tile_width_px = 0;
   int32_t _tile_height_px = 0;
   int32_t _tile_count = 0;
   int32_t _columns = 0;
   int32_t _rows = 0;

   TmxImage* _image = nullptr;
   std::map<int, TmxTile*> _tile_map;
   std::filesystem::path _path;
};

