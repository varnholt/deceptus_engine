#include "tmxpolygon.h"

#include "framework/tools/log.h"
#include "tmxtools.h"

#include <iostream>
#include <sstream>


void TmxPolygon::deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>&)
{
   const auto points = element->Attribute("points");
   const auto pairs = TmxTools::split(points, ' ');

   for (const auto& pair : pairs)
   {
      const auto two_points = TmxTools::splitPair(pair, ',');
      if (two_points.size() == 2)
      {
         const auto x = std::stof(two_points.at(0));
         const auto y = std::stof(two_points.at(1));

         _polyline.push_back(sf::Vector2f(x, y));
      }
      else
      {
         Log::Error() << "bad polygon data";
         break;
      }
   }
}


