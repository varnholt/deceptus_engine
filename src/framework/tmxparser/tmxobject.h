#pragma once

#include "tmxelement.h"

#include <memory>
#include <optional>

struct TmxPolygon;
struct TmxPolyLine;
struct TmxProperties;

struct TmxObject : TmxElement
{
   TmxObject() = default;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;

   std::string _id;
   float _x_px = 0.0f;
   float _y_px = 0.0f;
   float _width_px = 0.0f;
   float _height_px = 0.0f;

   std::optional<std::string> _template_name;
   std::optional<std::string> _template_type;
   std::optional<std::string> _gid;

   std::shared_ptr<TmxPolygon> _polygon;
   std::shared_ptr<TmxPolyLine> _polyline ;
   std::shared_ptr<TmxProperties> _properties;
};

