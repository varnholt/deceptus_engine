#include "tmxproperties.h"

#include "framework/tools/log.h"
#include "tmxproperty.h"

#include <iostream>


void TmxProperties::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

   auto node = element->FirstChild();
   while (node != nullptr)
   {
      auto sub_element = node->ToElement();
      if (sub_element != nullptr)
      {
         TmxElement* element = nullptr;
         TmxProperty* property = nullptr;

         if (sub_element->Name() == std::string("property"))
         {
            property = new TmxProperty();
            element = property;
         }

         if (element != nullptr)
         {
            element->deserialize(sub_element);
         }
         else
         {
            Log::Error() << sub_element->Name() << " is not supported for TmxProperties";
         }

         if (property != nullptr)
         {
            _map[property->_name] = property;
         }
      }

      node = node->NextSibling();
   }
}

