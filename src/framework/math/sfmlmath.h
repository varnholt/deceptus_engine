#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <math.h>
#include <optional>

namespace SfmlMath
{

float length(const sf::Vector2f&);
float lengthSquared(const sf::Vector2f&);
sf::Vector2f normalize(const sf::Vector2f& v);

template <typename T>
std::optional<T> intersect(
   const T& p0,
   const T& p1,
   const T& p2,
   const T& p3
)
{
   auto s1 = p1 - p0;
   auto s2 = p3 - p2;

   const auto sd = (-s2.x * s1.y + s1.x * s2.y);
   const auto td = (-s2.x * s1.y + s1.x * s2.y);

   if ( fabs(sd) < 0.0000001f || fabs(td) < 0.00000001f)
   {
      return {};
   }

   auto s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / sd;
   auto t = ( s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / td;

   if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
   {
       return T{p0.x + (t * s1.x), p0.y + (t * s1.y)};
   }

   return {};
}

template <typename T>
bool intersectCircleRect(const sf::Vector2<T>& circle_pos, T radius, const sf::Rect<T>& rect)
{
   auto test_x = circle_pos.x;
   auto test_y = circle_pos.y;

   if (circle_pos.x < rect.left)
   {
      test_x = rect.left;
   }
   else if (circle_pos.x > rect.left + rect.width)
   {
      test_x = rect.left + rect.width;
   }

   if (circle_pos.y < rect.top)
   {
      test_y = rect.top;
   }
   else if (circle_pos.y > rect.top + rect.height)
   {
      test_y = rect.top + rect.height;
   }

   const auto distance_x = circle_pos.x - test_x;
   const auto distance_y = circle_pos.y - test_y;
   const auto distance_squared = (distance_x * distance_x) + (distance_y * distance_y);

   return (distance_squared <= radius * radius);
}


}


