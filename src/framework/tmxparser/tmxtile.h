#pragma once

#include "tmxelement.h"

struct TmxAnimation;
struct TmxObjectGroup;

struct TmxTile : TmxElement
{
   TmxTile() = default;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   int32_t _id = 0;

   std::shared_ptr<TmxAnimation> _animation = nullptr;
   std::shared_ptr<TmxObjectGroup> _object_group = nullptr;
};

