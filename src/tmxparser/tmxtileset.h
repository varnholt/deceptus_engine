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

   void deserialize(tinyxml2::XMLElement* e) override;
   void parseTileSet(tinyxml2::XMLElement* element);

   std::string mSource;
   int32_t mFirstGid = 0;
   int32_t mTileWidth = 0;
   int32_t mTileHeight = 0;
   int32_t mTileCount = 0;
   int32_t mColumns = 0;
   int32_t mRows = 0;

   TmxImage* mImage = nullptr;
   std::map<int, TmxTile*> mTileMap;
   std::filesystem::path mPath;
};

