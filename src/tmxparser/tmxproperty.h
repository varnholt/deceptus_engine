#pragma once

#include "tmxelement.h"

struct TmxProperty : TmxElement
{
public:

   TmxProperty() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   std::string mType;
   std::string mValueStr;
   float mValueFloat = 0.0f;
   int mValueInt = 0;
};

