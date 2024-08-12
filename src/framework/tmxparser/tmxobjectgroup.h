#pragma once

// base
#include <map>
#include "tmxelement.h"

struct TmxObject;

struct TmxObjectGroup : TmxElement
{
   TmxObjectGroup();

   std::map<std::string, std::shared_ptr<TmxObject>> _objects;
   int _z_index = 0;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
};
