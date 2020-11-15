#include "tmxanimation.h"

#include "tmxframe.h"

TmxAnimation::TmxAnimation()
{
}


void TmxAnimation::deserialize(tinyxml2::XMLElement *element)
{
   tinyxml2::XMLNode* node = element->FirstChild();
   while(node != nullptr)
   {
      tinyxml2::XMLElement* subElement = node->ToElement();
      if (subElement != nullptr)
      {
         if (subElement->Name() == std::string("frame"))
         {
            TmxFrame* frame = new TmxFrame();
            mFrames.push_back(frame);
            frame->deserialize(subElement);
         }
      }

      node = node->NextSibling();
   }

//   if (mFrames.size() > 0)
//   {
//      printf("      %zd animation frames added\n", mFrames.size());
//   }
}
