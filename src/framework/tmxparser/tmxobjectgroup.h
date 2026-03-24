#pragma once

// base
#include <map>
#include "tmxelement.h"

struct TmxObject;

///
/// \brief Represents a TMX object group and its parsed object map.
///
struct TmxObjectGroup : TmxElement
{
   ///
   /// \brief Constructs an object group and sets the element type.
   ///
   TmxObjectGroup();

   std::map<std::string, std::shared_ptr<TmxObject>> _objects;
   int _z_index = 0;

   ///
   /// \brief Deserializes `<object>` children into `_objects`.
   /// \param e XML element for `<objectgroup>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;
};
