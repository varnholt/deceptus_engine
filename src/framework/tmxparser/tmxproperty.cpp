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
   mValueType = attrVal ? attrVal : "string";

   if (mValueType == "int")
   {
      mValueInt = element->IntAttribute("value");
   }
   if (mValueType == "bool")
   {
      mValueBool = element->BoolAttribute("value");
   }
   if (mValueType == "float")
   {
      mValueFloat = element->FloatAttribute("value");
   }
   else if (mValueType == "string" )
   {
      mValueStr = element->Attribute("value");
   }
   else if (mValueType == "color")
   {
      mValueStr = element->Attribute("value");
   }
}

