#pragma once

#include "tmxelement.h"

struct TmxFrame;

struct TmxAnimation : TmxElement
{
   TmxAnimation() = default;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
   std::vector<std::shared_ptr<TmxFrame>> _frames;
};
