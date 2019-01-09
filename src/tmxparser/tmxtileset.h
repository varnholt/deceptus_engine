#pragma once

// base
#include "tmxelement.h"

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
   int mFirstGid = 0;
   int mTileWidth = 0;
   int mTileHeight = 0;
   int mTileCount = 0;
   int mColumns = 0;
   int mRows = 0;

   TmxImage* mImage = nullptr;
   std::map<int, TmxTile*> mTileMap;
   std::filesystem::path mPath;
};

