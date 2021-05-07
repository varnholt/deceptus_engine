#include "tmxanimation.h"

#include "tmxframe.h"


TmxAnimation::~TmxAnimation()
{
   _frames.clear();
}


void TmxAnimation::deserialize(tinyxml2::XMLElement *element)
{
   tinyxml2::XMLNode* node = element->FirstChild();
   while(node != nullptr)
   {
      tinyxml2::XMLElement* sub_element = node->ToElement();
      if (sub_element != nullptr)
      {
         if (sub_element->Name() == std::string("frame"))
         {
            TmxFrame* frame = new TmxFrame();
            _frames.push_back(frame);
            frame->deserialize(sub_element);
         }
      }

      node = node->NextSibling();
   }

//   if (mFrames.size() > 0)
//   {
//      printf("      %zd animation frames added\n", mFrames.size());
//   }
}
