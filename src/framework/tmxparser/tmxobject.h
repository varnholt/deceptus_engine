#pragma once

#include "tmxelement.h"

struct TmxPolygon;
struct TmxPolyLine;
struct TmxProperties;

struct TmxObject : TmxElement
{
   TmxObject() = default;
   ~TmxObject() override;

   void deserialize(tinyxml2::XMLElement* e) override;

   std::string _id;
   float _x_px = 0.0f;
   float _y_px = 0.0f;
   float _width_px = 0.0f;
   float _height_px = 0.0f;

   TmxPolygon* _polygon = nullptr;
   TmxPolyLine* _polyline = nullptr;
   TmxProperties* _properties = nullptr;
};

