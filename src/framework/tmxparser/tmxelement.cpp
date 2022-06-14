#include "tmxelement.h"


void TmxElement::deserialize(tinyxml2::XMLElement*e, const std::shared_ptr<TmxParseData>& parse_data)
{
   _name = (e->Attribute("name") == nullptr) ? "" : e->Attribute("name");
   _parse_data = parse_data;
}
