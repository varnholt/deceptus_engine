#pragma once

#include "tmxelement.h"

#include <map>

struct TmxProperty;

///
/// \brief Represents a named map of TMX `<property>` elements.
///
struct TmxProperties : public TmxElement
{
public:
   TmxProperties() = default;

   ///
   /// \brief Parses child `<property>` elements into `_map`.
   /// \param e XML element for `<properties>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;
   std::map<std::string, std::shared_ptr<TmxProperty>> _map;
};
