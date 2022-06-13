#include "tmxproperty.h"

#include <sstream>

/*

  <properties>
   <property name="z" type="int" value="1"/>
  </properties>

*/


void TmxProperty::deserialize(tinyxml2::XMLElement *element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   auto attrVal = element->Attribute("type");
   _value_type = attrVal ? attrVal : "string";

   if (_value_type == "int")
   {
      _value_int = element->IntAttribute("value");
   }
   if (_value_type == "bool")
   {
      _value_bool = element->BoolAttribute("value");
   }
   if (_value_type == "float")
   {
      _value_float = element->FloatAttribute("value");
   }
   else if (_value_type == "string")
   {
      _value_string = element->Attribute("value");
   }
   else if (_value_type == "color")
   {
      _value_string = element->Attribute("value");
   }
}


std::string TmxProperty::toString() const
{
   if (_value_type == "string" )
   {
      return _value_string.value();
   }
   else if (_value_type == "float")
   {
      std::ostringstream stream;
      stream << _value_float.value();
      return stream.str();
   }
   else if (_value_type == "int")
   {
      std::ostringstream stream;
      stream << _value_int.value();
      return stream.str();
   }
   else if (_value_type == "bool")
   {
      std::ostringstream stream;
      stream << _value_bool.value();
      return stream.str();
   }

   return {};
}

