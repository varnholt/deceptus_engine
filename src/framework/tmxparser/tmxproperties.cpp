#include "tmxproperties.h"
#include "tmxproperty.h"


void TmxProperties::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

   auto node = element->FirstChild();
   while (node != nullptr)
   {
      auto subElement = node->ToElement();
      if (subElement != nullptr)
      {
         TmxElement* element = nullptr;
         TmxProperty* property = nullptr;

         if (subElement->Name() == std::string("property"))
         {
            property = new TmxProperty();
            element = property;
         }

         if (element != nullptr)
         {
            element->deserialize(subElement);
         }
         else
         {
            printf(
               "%s is not supported for TmxProperties\n",
               subElement->Name()
            );
         }

         if (property != nullptr)
         {
            _map[property->_name] = property;
         }
      }

      node = node->NextSibling();
   }
}

