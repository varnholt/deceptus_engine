#pragma once

// base
#include "tmxelement.h"
#include <map>

struct TmxObject;

struct TmxObjectGroup : TmxElement
{
   std::map<std::string, TmxObject*> mObjects;
   int mZ = 0;

   TmxObjectGroup();
   void deserialize(tinyxml2::XMLElement* e) override;
};

