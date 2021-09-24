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

   _id = element->IntAttribute("id");

   // std::cout << "   tile (id: " << mId << ")" << std::endl;

   auto node = element->FirstChild();
   while (node != nullptr)
   {
      auto child_element = node->ToElement();
      if (child_element != nullptr)
      {
         TmxElement* tmp = nullptr;

         if (child_element->Name() == std::string("animation"))
         {
            _animation = new TmxAnimation();
            tmp = _animation;
         }
         else if (child_element->Name() == std::string("objectgroup"))
         {
           _object_group = new TmxObjectGroup();
           tmp = _object_group;
         }

         if (tmp != nullptr)
         {
            tmp->deserialize(child_element);
         }
         else
         {
            std::cerr << child_element->Name() << " is not supported for TmxTile" << std::endl;
         }
      }

      node = node->NextSibling();
   }
}
