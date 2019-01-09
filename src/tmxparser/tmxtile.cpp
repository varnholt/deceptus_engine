#include "tmxtile.h"

//
#include "tmxanimation.h"
#include "tmxobjectgroup.h"
#include <iostream>

TmxTile::TmxTile()
 : mId(0),
   mAnimation(nullptr)
{
}


void TmxTile::deserialize(tinyxml2::XMLElement* element)
{
   TmxElement::deserialize(element);

   mId = element->IntAttribute("id");

   // std::cout << "   tile (id: " << mId << ")" << std::endl;

   tinyxml2::XMLNode* node = element->FirstChild();
   while (node != nullptr)
   {
      tinyxml2::XMLElement* childElement = node->ToElement();
      if (childElement != nullptr)
      {
         TmxElement* tmp = nullptr;

         if (childElement->Name() == std::string("animation"))
         {
            mAnimation = new TmxAnimation();
            tmp = mAnimation;
         }
         else if (childElement->Name() == std::string("objectgroup"))
         {
           mObjectGroup = new TmxObjectGroup();
           tmp = mObjectGroup;
         }

         if (tmp != nullptr)
         {
            tmp->deserialize(childElement);
         }
         else
         {
            printf(
               "%s is not supported for TmxTile\n",
               childElement->Name()
            );
         }
      }

      node = node->NextSibling();
   }
}
