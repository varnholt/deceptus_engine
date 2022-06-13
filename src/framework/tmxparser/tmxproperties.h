#pragma once

#include "tmxelement.h"

#include <map>

struct TmxProperty;


struct TmxProperties : public TmxElement
{
public:
   TmxProperties() = default;
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;
   std::map<std::string, TmxProperty*> _map;
};

