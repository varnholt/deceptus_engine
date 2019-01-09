#pragma once

#include <string>
#include <vector>
#include "tinyxml2/tinyxml2.h"

struct TmxElement
{
   enum Type {
      TypeInvalid = 0,
      TypeTileSet = 1,
      TypeLayer = 2,
      TypeObjectGroup = 3,
      TypeImageLayer = 4
   };


   TmxElement() = default;
   virtual ~TmxElement() = default;


   virtual void deserialize(tinyxml2::XMLElement* e);
   std::string mName;

   Type mType;
};
