#pragma once

#include "tmxelement.h"

struct TmxAnimation;
struct TmxObjectGroup;

struct TmxTile : TmxElement
{
   TmxTile() = default;
   ~TmxTile() override;

   void deserialize(tinyxml2::XMLElement* e) override;

   int mId = 0;

   TmxAnimation* _animation = nullptr;
   TmxObjectGroup* _object_group = nullptr;
};

