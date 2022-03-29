#pragma once

#include "tmxelement.h"


struct TmxFrame;


struct TmxAnimation : TmxElement
{
   TmxAnimation() = default;
   virtual ~TmxAnimation();

   void deserialize(tinyxml2::XMLElement* e) override;
   std::vector<TmxFrame*> _frames;
};

