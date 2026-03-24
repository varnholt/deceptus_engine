#pragma once

#include <memory>
#include <string>
#include <vector>

#include "tinyxml2/tinyxml2.h"

struct TmxElement;
struct TmxLayer;
struct TmxObjectGroup;
struct TmxParseData;
struct TmxTileSet;

///
/// \brief Parses TMX map files into runtime TMX element objects.
///
class TmxParser
{
public:
   TmxParser() = default;

   ///
   /// \brief Loads and parses a TMX file into `_elements`.
   /// \param filename TMX file path.
   ///
   void parse(const std::string& filename);
   ///
   /// \brief Returns all parsed top-level and flattened group elements.
   /// \return Parsed TMX elements.
   ///
   const std::vector<std::shared_ptr<TmxElement>>& getElements() const;
   ///
   /// \brief Returns only parsed object-group elements.
   /// \return Parsed object groups.
   ///
   std::vector<std::shared_ptr<TmxObjectGroup>> retrieveObjectGroups() const;
   ///
   /// \brief Finds the tileset matching the highest gid referenced by a layer.
   /// \param layer Tile layer to resolve against loaded tilesets.
   /// \return Matching tileset, or `nullptr` when no match exists.
   ///
   std::shared_ptr<TmxTileSet> getTileSet(const std::shared_ptr<TmxLayer>& layer) const;

protected:
   std::vector<std::shared_ptr<TmxElement>> _elements;
   ///
   /// \brief Parses child elements of a TMX `<group>` and appends them to `_elements`.
   /// \param sub_element Group element to traverse.
   /// \param z Current z value passed through nested parsing.
   ///
   void parseGroup(tinyxml2::XMLElement* sub_element, int32_t& z);
   ///
   /// \brief Parses one TMX element node into its concrete runtime type.
   /// \param sub_element Element node to parse.
   /// \param z Current z value used for layer/image/object ordering.
   ///
   void parseSubElement(tinyxml2::XMLElement* sub_element, int32_t& z);

   std::shared_ptr<TmxParseData> _parse_data;
};
