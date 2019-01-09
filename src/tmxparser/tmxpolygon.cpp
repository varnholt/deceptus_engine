#include "tmxpolygon.h"

#include <sstream>
#include "tmxtools.h"

// polygon points="0,0 24,0 24,24 8,24 8,16 0,16"/>


void TmxPolygon::deserialize(tinyxml2::XMLElement *element)
{
   std::string points = element->Attribute("points");
   std::vector<std::string> pairs = TmxTools::split(points, ' ');

  //   printf("polyline (number of points: %zd)\n",
  //      pairs.size()
  //   );

   for (std::string pair : pairs)
   {
      std::vector<std::string> twoPoints = TmxTools::split(pair, ',');
      if (twoPoints.size() == 2)
      {
         float x = std::stof(twoPoints.at(0));
         float y = std::stof(twoPoints.at(1));

         mPolyLine.push_back(sf::Vector2f(x, y));
      }
      else
      {
         printf("bad polygon data\n");
         break;
      }
   }
}


