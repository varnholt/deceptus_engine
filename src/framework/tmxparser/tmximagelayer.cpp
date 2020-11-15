#include "tmximagelayer.h"
#include "tmximage.h"
#include "tmxproperties.h"



TmxImageLayer::TmxImageLayer()
{
  mType = TypeImageLayer;
}


void TmxImageLayer::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

   mOpacity = element->FloatAttribute("opacity", 1.0f);
   mOffsetX = element->FloatAttribute("offsetx", 0.0f);
   mOffsetY = element->FloatAttribute("offsety", 0.0f);

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
      tinyxml2::XMLElement* subElement = node->ToElement();
      if (subElement != nullptr)
      {
         TmxElement* element = nullptr;
         auto parsed = false;

         if (subElement->Name() == std::string("image"))
         {
            mImage = new TmxImage();
            element = mImage;
         }
         else if (subElement->Name() == std::string("properties"))
         {
            mProperties = new TmxProperties();
            mProperties->deserialize(subElement);
            parsed = true;
         }

        if (element != nullptr)
        {
           element->deserialize(subElement);
        }
        else if (!parsed)
        {
           printf(
              "%s is not supported for TmxElement\n",
              subElement->Name()
           );
        }
     }

     node = node->NextSibling();
  }

}
