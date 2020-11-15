#pragma once

#include "tmxelement.h"


struct TmxFrame;


struct TmxAnimation : TmxElement
{
   TmxAnimation();
   void deserialize(tinyxml2::XMLElement* e);
   std::vector<TmxFrame*> mFrames;
};

