#pragma once

#include "tmxelement.h"

struct TmxImage;
struct TmxProperties;

struct TmxImageLayer : public TmxElement
{
   TmxImageLayer();

   float _offset_x_px = 0.0f;
   float _offset_y_px = 0.0f;
   float _opacity = 1.0f;
   std::shared_ptr<TmxImage> _image;
   std::shared_ptr<TmxProperties> _properties;
   int32_t _z = 0;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
};
