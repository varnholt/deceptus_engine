#include "tmxelement.h"


void TmxElement::deserialize(tinyxml2::XMLElement *e)
{
   _name = (e->Attribute("name") == nullptr) ? "" : e->Attribute("name");
}
