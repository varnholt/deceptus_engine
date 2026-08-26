#pragma once

#include <map>
#include <memory>
#include <string>

struct TmxObject;

///
/// \brief Carries shared parse context for TMX element deserialization.
///
struct TmxParseData
{
   std::string _filename;

   //! objects loaded from tmx templates, keyed by the template path as written in the map
   std::map<std::string, std::shared_ptr<TmxObject>> _template_objects;
};
