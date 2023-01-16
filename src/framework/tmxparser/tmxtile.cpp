#include "tmxtile.h"

#include "framework/tools/log.h"
#include "tmxanimation.h"
#include "tmxobjectgroup.h"

#include <iostream>


void TmxTile::deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   _id = element->IntAttribute("id");

   auto node = element->FirstChild();
   while (node)
   {
      auto child_element = node->ToElement();
      if (child_element)
      {
         std::shared_ptr<TmxElement> tmp;

         if (child_element->Name() == std::string("animation"))
         {
            _animation = std::make_shared<TmxAnimation>();
            tmp = _animation;
         }
         else if (child_element->Name() == std::string("objectgroup"))
         {
           _object_group = std::make_shared<TmxObjectGroup>();
           tmp = _object_group;
         }

         if (tmp)
         {
            tmp->deserialize(child_element, parse_data);
         }
         else
         {
            Log::Error() << child_element->Name() << " is not supported for TmxTile";
         }
      }

      node = node->NextSibling();
   }
}
