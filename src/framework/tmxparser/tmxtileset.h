#pragma once

// base
#include "tmxelement.h"

#include <cstdint>
#include <filesystem>
#include <map>

// forward declarations
struct TmxImage;
struct TmxTile;

///
/// \brief Represents a TMX tileset and its optional external source file.
///
struct TmxTileSet : TmxElement
{
   ///
   /// \brief Constructs a tileset and sets the element type.
   ///
   TmxTileSet();

   ///
   /// \brief Deserializes tileset metadata and tile/image children.
   /// \param e XML element for `<tileset>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
   ///
   /// \brief Parses a concrete tileset element into this object.
   /// \param element XML element containing tileset fields.
   /// \param parse_data Shared TMX parse context.
   ///
   void parseTileSet(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>&);

   std::string _source;
   int32_t _first_gid = 0;
   int32_t _tile_width_px = 0;
   int32_t _tile_height_px = 0;
   int32_t _tile_count = 0;
   int32_t _columns = 0;
   int32_t _rows = 0;

   std::shared_ptr<TmxImage> _image;
   std::map<int, std::shared_ptr<TmxTile>> _tile_map;
   std::filesystem::path _path;
};
