#include "tmxproperty.h"

#include <sstream>

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


std::string TmxProperty::toString() const
{
   if (mValueStr.has_value())
   {
      return mValueStr.value();
   }
   else if (mValueFloat.has_value())
   {
      std::ostringstream stream;
      stream << mValueFloat.value();
      return stream.str();
   }
   else if (mValueInt.has_value())
   {
      std::ostringstream stream;
      stream << mValueInt.value();
      return stream.str();
   }
   else if (mValueBool.has_value())
   {
      std::ostringstream stream;
      stream << mValueBool.value();
      return stream.str();
   }

   return {};
}

