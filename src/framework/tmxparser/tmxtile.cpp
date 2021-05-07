#include "tmxtile.h"

//
#include "tmxanimation.h"
#include "tmxobjectgroup.h"
#include <iostream>


TmxTile::~TmxTile()
{
   delete _animation;
   delete _object_group;
}

void TmxTile::deserialize(tinyxml2::XMLElement* element)
{
   TmxElement::deserialize(element);

   mId = element->IntAttribute("id");

   // std::cout << "   tile (id: " << mId << ")" << std::endl;

   auto node = element->FirstChild();
   while (node != nullptr)
   {
      auto childElement = node->ToElement();
      if (childElement != nullptr)
      {
         TmxElement* tmp = nullptr;

         if (childElement->Name() == std::string("animation"))
         {
            _animation = new TmxAnimation();
            tmp = _animation;
         }
         else if (childElement->Name() == std::string("objectgroup"))
         {
           _object_group = new TmxObjectGroup();
           tmp = _object_group;
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
