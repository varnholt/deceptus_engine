#pragma once

#include "tmxelement.h"

struct TmxImage : TmxElement
{
   TmxImage() = default;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;

   std::string _source;
   int _width_px = 0;
   int _height_px = 0;
};
