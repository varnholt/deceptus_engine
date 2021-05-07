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
   int _z = 0;

   void deserialize(tinyxml2::XMLElement* e) override;
};

