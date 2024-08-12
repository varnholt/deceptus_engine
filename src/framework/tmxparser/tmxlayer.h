#pragma once

#include "tmxchunk.h"
#include "tmxelement.h"

struct TmxProperties;

struct TmxLayer : TmxElement
{
   TmxLayer();

   void deserialize(tinyxml2::XMLElement*, const std::shared_ptr<TmxParseData>&) override;

   uint32_t _width_tl = 0;
   uint32_t _height_tl = 0;
   float _opacity = 1.0f;
   bool _visible = true;
   std::shared_ptr<TmxProperties> _properties = nullptr;
   int32_t _z = 0;

   int32_t _offset_x_px = 0;
   int32_t _offset_y_px = 0;

   std::vector<int32_t> _data;
};
