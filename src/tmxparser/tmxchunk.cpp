#include "tmxchunk.h"

#include <iostream>
#include <sstream>
#include "tmxtools.h"


TmxChunk::~TmxChunk()
{
  delete mData;
}


void TmxChunk::deserialize(tinyxml2::XMLElement *element)
{
   TmxElement::deserialize(element);

   mX = element->IntAttribute("x");
   mY = element->IntAttribute("y");
   mWidth = element->IntAttribute("width");
   mHeight = element->IntAttribute("height");

   mData = new int32_t[mWidth * mHeight];

   std::string data = element->FirstChild()->Value();

   // parse csv data and store it in mData array
   std::stringstream stream(data);
   std::string line;

   auto y = 0;
   while (std::getline(stream, line, '\n'))
   {
      TmxTools::trim(line);

      if (line.empty())
         continue;

      auto x = 0;
      const auto rowContent = TmxTools::split(line, ',');

      for (const auto& valStr : rowContent)
      {
         auto val = std::stoi(valStr);
         mData[y * mWidth + x] = val;
         x++;
      }

      y++;
   }
}


