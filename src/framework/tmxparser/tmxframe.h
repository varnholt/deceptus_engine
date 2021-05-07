#pragma once

#include "tmxelement.h"

struct TmxFrame : TmxElement
{
   TmxFrame() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   int _tile_id = 0;
   int _duration_ms = 0;
};

