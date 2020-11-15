#pragma once

#include "tmxelement.h"

struct TmxPolygon;
struct TmxPolyLine;
struct TmxProperties;

struct TmxObject : TmxElement
{
   TmxObject() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   std::string mId;
   float mX = 0.0f;
   float mY = 0.0f;
   float mWidth = 0.0f;
   float mHeight = 0.0f;

   TmxPolygon* mPolygon = nullptr;
   TmxPolyLine* mPolyLine = nullptr;
   TmxProperties* mProperties = nullptr;
};

