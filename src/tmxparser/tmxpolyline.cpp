#include "tmxpolyline.h"

#include <sstream>
#include "tmxtools.h"


TmxPolyLine::TmxPolyLine()
{
}


void TmxPolyLine::deserialize(tinyxml2::XMLElement *element)
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


//<object id="42" name="walkpoly1" x="384.5" y="265">
// <polyline points="0,0 22,-0.5 21.5,96 93.5,96.5 94.5,169.5 218,168.5 218.5,97.5 291.5,97.5 290,-1.5 309.5,-1.5 310,118.5 236.5,117.5 237.5,189 74.5,189.5 74.5,117 1,118 0,3"/>
//</object>
