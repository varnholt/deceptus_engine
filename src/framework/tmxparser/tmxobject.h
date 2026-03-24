#pragma once

#include "tmxelement.h"

#include <memory>
#include <optional>

struct TmxPolygon;
struct TmxPolyLine;
struct TmxProperties;

///
/// \brief Represents one TMX object entry with optional geometry and properties.
///
struct TmxObject : TmxElement
{
   ///
   /// \brief Constructs an empty object.
   ///
   TmxObject() = default;

   ///
   /// \brief Parses object attributes and optional child elements.
   /// \param e XML element for `<object>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   std::string _id;
   float _x_px = 0.0f;
   float _y_px = 0.0f;
   float _width_px = 0.0f;
   float _height_px = 0.0f;

   std::optional<std::string> _template_name;
   std::optional<std::string> _template_type;
   std::optional<std::string> _gid;

   std::shared_ptr<TmxPolygon> _polygon;
   std::shared_ptr<TmxPolyLine> _polyline;
   std::shared_ptr<TmxProperties> _properties;
};
