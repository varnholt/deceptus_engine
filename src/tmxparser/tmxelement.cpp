#include "tmxelement.h"


void TmxElement::deserialize(tinyxml2::XMLElement *e)
{
   mName = (e->Attribute("name") == nullptr) ? "" : e->Attribute("name");
}
