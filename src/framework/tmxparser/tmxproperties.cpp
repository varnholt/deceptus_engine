#include "tmxproperties.h"

#include "framework/tools/log.h"
#include "tmxproperty.h"

#include <iostream>


void TmxProperties::deserialize(tinyxml2::XMLElement *element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   auto node = element->FirstChild();
   while (node)
   {
      auto sub_element = node->ToElement();
      if (!sub_element)
      {
         node = node->NextSibling();
         continue;
      }

      TmxElement* next_element = nullptr;
      TmxProperty* property = nullptr;

      if (sub_element->Name() == std::string("property"))
      {
         property = new TmxProperty();
         next_element = property;
      }

      if (next_element)
      {
         next_element->deserialize(sub_element, parse_data);
      }
      else
      {
         Log::Error() << sub_element->Name() << " is not supported for TmxProperties";
      }

      if (property)
      {
         _map[property->_name] = property;
      }

      node = node->NextSibling();
   }
}

