#pragma once

#include "tmxelement.h"

struct TmxAnimation;
struct TmxObjectGroup;

struct TmxTile : TmxElement
{
   TmxTile();

   void deserialize(tinyxml2::XMLElement* e) override;

   int mId = 0;

   TmxAnimation* mAnimation = nullptr;
   TmxObjectGroup* mObjectGroup = nullptr;
};

