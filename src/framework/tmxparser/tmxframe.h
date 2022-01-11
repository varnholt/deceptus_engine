#pragma once

#include "tmxelement.h"

struct TmxFrame : TmxElement
{
   TmxFrame() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   int32_t _tile_id = 0;
   int32_t _duration_ms = 0;
};

