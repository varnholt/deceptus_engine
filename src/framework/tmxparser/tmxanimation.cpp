#include "tmxanimation.h"

#include "tmxframe.h"


void TmxAnimation::deserialize(tinyxml2::XMLElement *element, const std::shared_ptr<TmxParseData>& parse_data)
{
   auto* node = element->FirstChild();
   while (node)
   {
      auto* sub_element = node->ToElement();
      if (!sub_element)
      {
         node = node->NextSibling();
         continue;
      }

      if (sub_element->Name() == std::string("frame"))
      {
         auto frame = std::make_shared<TmxFrame>();
         _frames.push_back(frame);
         frame->deserialize(sub_element, parse_data);
      }

      node = node->NextSibling();
   }
}
