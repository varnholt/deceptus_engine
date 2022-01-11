#include "tmxanimation.h"

#include "tmxframe.h"


TmxAnimation::~TmxAnimation()
{
   _frames.clear();
}


void TmxAnimation::deserialize(tinyxml2::XMLElement *element)
{
   auto node = element->FirstChild();
   while (node)
   {
      auto sub_element = node->ToElement();
      if (!sub_element)
      {
         node = node->NextSibling();
         continue;
      }

      if (sub_element->Name() == std::string("frame"))
      {
         auto frame = new TmxFrame();
         _frames.push_back(frame);
         frame->deserialize(sub_element);
      }

      node = node->NextSibling();
   }
}
