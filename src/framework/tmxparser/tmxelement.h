#pragma once

#include <memory>
#include <string>
#include <vector>
#include "tinyxml2/tinyxml2.h"

struct TmxParseData;

///
/// \brief Base type for all parsed TMX elements.
///
struct TmxElement
{
   enum class Type
   {
      TypeInvalid = 0,
      TypeTileSet = 1,
      TypeLayer = 2,
      TypeObjectGroup = 3,
      TypeImageLayer = 4,
      TypeTemplate = 5
   };

   TmxElement() = default;

   ///
   /// \brief Destroys the element.
   ///
   virtual ~TmxElement() = default;

   ///
   /// \brief Reads common TMX fields shared by derived elements.
   /// \param e XML element being parsed.
   /// \param parse_data Shared parse context.
   ///
   virtual void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data);

   std::string _name;

   Type _type = Type::TypeInvalid;
   std::shared_ptr<TmxParseData> _parse_data;
};
