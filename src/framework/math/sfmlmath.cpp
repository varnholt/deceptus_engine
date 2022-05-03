#include "sfmlmath.h"

#include <math.h>


float SfmlMath::length(const sf::Vector2f & v)
{
   return sqrt(v.x * v.x + v.y * v.y);
}


float SfmlMath::lengthSquared(const sf::Vector2f & v)
{
   return v.x * v.x + v.y * v.y;
}


sf::Vector2f SfmlMath::normalize(const sf::Vector2f& v)
{
   auto len = length(v);
   if (len > 0.0f)
   {
      return {v.x / len, v.y / len};
   }
   else
   {
      return {};
   }
}

