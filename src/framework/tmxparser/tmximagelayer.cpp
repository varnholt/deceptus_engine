#include "tmximagelayer.h"

#include "framework/tools/log.h"
#include "tmximage.h"
#include "tmxproperties.h"


#include <iostream>


TmxImageLayer::TmxImageLayer()
{
   _type = TypeImageLayer;
}


TmxImageLayer::~TmxImageLayer()
{
   delete _image;
   delete _properties;
}


void TmxImageLayer::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

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

      TmxElement* element = nullptr;
      auto parsed = false;

      if (sub_element->Name() == std::string("image"))
      {
         _image = new TmxImage();
         element = _image;
      }
      else if (sub_element->Name() == std::string("properties"))
      {
         _properties = new TmxProperties();
         _properties->deserialize(sub_element);
         parsed = true;
      }

      if (element)
      {
         element->deserialize(sub_element);
      }
      else if (!parsed)
      {
         Log::Error() << sub_element->Name() << " is not supported for TmxElement";
      }

      node = node->NextSibling();
   }
}
