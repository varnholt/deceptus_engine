#pragma once

#include "tmxelement.h"

#include <optional>

struct TmxPolygon;
struct TmxPolyLine;
struct TmxProperties;

struct TmxObject : TmxElement
{
   TmxObject() = default;
   ~TmxObject() override;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;

   std::string _id;
   float _x_px = 0.0f;
   float _y_px = 0.0f;
   float _width_px = 0.0f;
   float _height_px = 0.0f;
   std::optional<std::string> _template_name;
   std::optional<std::string> _type;

   TmxPolygon* _polygon = nullptr;
   TmxPolyLine* _polyline = nullptr;
   TmxProperties* _properties = nullptr;
};

