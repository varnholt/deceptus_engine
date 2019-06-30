#include "tmxproperty.h"

/*

  <properties>
   <property name="z" type="int" value="1"/>
  </properties>

*/


void TmxProperty::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

   auto attrVal = element->Attribute("type");
   mType = attrVal ? attrVal : "string";

   if (mType == "int")
   {
      mValueInt = element->IntAttribute("value");
   }
   if (mType == "float")
   {
      mValueFloat = element->FloatAttribute("value");
   }
   else if (mType == "string" )
   {
      mValueStr = element->Attribute("value");
   }
   else if (mType == "color")
   {
      mValueStr = element->Attribute("value");
   }
}

