#include "tmximagelayer.h"
#include "tmximage.h"
#include "tmxproperties.h"



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

   //  printf(
   //     "image layer: %s (opacity: %f, offset: %f, %f)\n",
   //     mName.c_str(),
   //     mOpacity,
   //     mOffsetX,
   //     mOffsetY
   //  );

   tinyxml2::XMLNode* node = element->FirstChild();
   while(node != nullptr)
   {
      tinyxml2::XMLElement* sub_element = node->ToElement();
      if (sub_element != nullptr)
      {
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

         if (element != nullptr)
         {
            element->deserialize(sub_element);
         }
         else if (!parsed)
         {
            printf(
              "%s is not supported for TmxElement\n",
              sub_element->Name()
            );
         }
      }

      node = node->NextSibling();
   }
}
