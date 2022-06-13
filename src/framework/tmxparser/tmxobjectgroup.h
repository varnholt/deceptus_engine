#pragma once

// base
#include "tmxelement.h"
#include <map>

struct TmxObject;

struct TmxObjectGroup : TmxElement
{
   TmxObjectGroup();
   ~TmxObjectGroup() override;

   std::map<std::string, TmxObject*> _objects;
   int _z_index = 0;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
};

