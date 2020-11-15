#include "tmxpolyline.h"

#include <sstream>
#include "tmxtools.h"

void TmxPolyLine::deserialize(tinyxml2::XMLElement *element)
{
   const auto points = element->Attribute("points");
   const auto pairs = TmxTools::split(points, ' ');

   for (auto& pair : pairs)
   {
      const auto twoPoints = TmxTools::splitPair(pair, ',');
      if (twoPoints.size() == 2)
      {
         const auto x = std::stof(twoPoints.at(0));
         const auto y = std::stof(twoPoints.at(1));

         mPolyLine.push_back(sf::Vector2f(x, y));
      }
      else
      {
         printf("bad polygon data\n");
         break;
      }
   }
}
