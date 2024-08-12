#include "tmxchunk.h"

#include <iostream>
#include <sstream>
#include "tmxtools.h"

TmxChunk::~TmxChunk()
{
   delete _data;
}

void TmxChunk::deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   _x_px = element->IntAttribute("x");
   _y_px = element->IntAttribute("y");
   _width_px = element->IntAttribute("width");
   _height_px = element->IntAttribute("height");

   _data = new int32_t[_width_px * _height_px];

   std::string data = element->FirstChild()->Value();

   // parse csv data and store it in mData array
   std::stringstream stream(data);
   std::string line;

   auto y = 0;
   while (std::getline(stream, line, '\n'))
   {
      TmxTools::trim(line);

      if (line.empty())
      {
         continue;
      }

      auto x = 0;
      const auto row_content = TmxTools::split(line, ',');

      for (const auto& val_string : row_content)
      {
         auto val = std::stoi(val_string);
         _data[y * _width_px + x] = val;
         x++;
      }

      y++;
   }
}
