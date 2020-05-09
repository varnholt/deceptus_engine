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
      const T& q0,
      const T& q1
   )
   {
      const auto x1 = p0.x;
      const auto x2 = p1.x;
      const auto x3 = q0.x;
      const auto x4 = q1.x;
      const auto y1 = p0.y;
      const auto y2 = p1.y;
      const auto y3 = q0.y;
      const auto y4 = q1.y;

      const auto det = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

      // no intersection
      if (fabs(det) < 0.000001f)
      {
         return {};
      }

      const auto pre  = (x1 * y2 - y1 * x2);
      const auto post = (x3 * y4 - y3 * x4);
      const auto x    = (pre * (x3 - x4) - (x1 - x2) * post) / det;
      const auto y    = (pre * (y3 - y4) - (y1 - y2) * post) / det;

      // check if x and y are within lines
      if (
             x < std::min(x1, x2)
          || x > std::max(x1, x2)
          || x < std::min(x3, x4)
          || x > std::max(x3, x4)
      )
      {
         return {};
      }

      if (
             y < std::min(y1, y2)
          || y > std::max(y1, y2)
          || y < std::min(y3, y4)
          || y > std::max(y3, y4)
      )
      {
         return {};
      }

      return T{x, y};
   }
};


