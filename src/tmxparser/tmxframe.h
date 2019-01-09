#pragma once

#include "tmxelement.h"

struct TmxFrame : TmxElement
{
   TmxFrame();

   void deserialize(tinyxml2::XMLElement* e) override;

   int mTileId;
   int mDuration;
};

