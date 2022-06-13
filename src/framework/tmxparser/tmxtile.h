#pragma once

#include "tmxelement.h"

struct TmxAnimation;
struct TmxObjectGroup;

struct TmxTile : TmxElement
{
   TmxTile() = default;
   ~TmxTile() override;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   int32_t _id = 0;

   TmxAnimation* _animation = nullptr;
   TmxObjectGroup* _object_group = nullptr;
};

