#pragma once

#include <memory>
#include <string>

struct TmxObject;
struct TmxParseData;

///
/// \brief Loads and exposes one object instance from a TMX template file.
///
struct TmxTemplate
{
   ///
   /// \brief Loads a template file and deserializes its contained object.
   /// \param filename Template file path relative to the source map.
   /// \param parse_data Shared TMX parse context.
   ///
   TmxTemplate(const std::string& filename, const std::shared_ptr<TmxParseData>& parse_data);

   std::shared_ptr<TmxObject> _object;
};
