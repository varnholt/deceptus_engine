#include "tmximagelayer.h"

#include "framework/tools/log.h"
#include "tmximage.h"
#include "tmxproperties.h"


#include <iostream>


TmxImageLayer::TmxImageLayer()
{
   _type = Type::TypeImageLayer;
}


void TmxImageLayer::deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   _opacity = element->FloatAttribute("opacity", 1.0f);
   _offset_x_px = element->FloatAttribute("offsetx", 0.0f);
   _offset_y_px = element->FloatAttribute("offsety", 0.0f);

   auto node = element->FirstChild();
   while (node)
   {
      auto sub_element = node->ToElement();
      if (!sub_element)
      {
         node = node->NextSibling();
         continue;
      }

      std::shared_ptr<TmxElement> next_element;
      auto parsed = false;

      if (sub_element->Name() == std::string("image"))
      {
         _image = std::make_shared<TmxImage>();
         next_element = _image;
      }
      else if (sub_element->Name() == std::string("properties"))
      {
         _properties = std::make_shared<TmxProperties>();
         _properties->deserialize(sub_element, parse_data);
         parsed = true;
      }

      if (next_element)
      {
         next_element->deserialize(sub_element, parse_data);
      }
      else if (!parsed)
      {
         Log::Error() << sub_element->Name() << " is not supported for TmxElement";
      }

      node = node->NextSibling();
   }
}
