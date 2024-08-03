#include "sfmlmath.h"

#include <cmath>

float SfmlMath::length(const sf::Vector2f& v)
{
   return sqrt(v.x * v.x + v.y * v.y);
}

float SfmlMath::lengthSquared(const sf::Vector2f& v)
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

   return {};
}

sf::Color mixColors(const sf::Color& color_1, const sf::Color& color_2, float ratio)
{
   const auto inv_ratio = 1.0f - ratio;

   const auto red = static_cast<uint8_t>(color_1.r * ratio + color_2.r * inv_ratio);
   const auto green = static_cast<uint8_t>(color_1.g * ratio + color_2.g * inv_ratio);
   const auto blue = static_cast<uint8_t>(color_1.b * ratio + color_2.b * inv_ratio);
   const auto alpha = static_cast<uint8_t>(color_1.a * ratio + color_2.a * inv_ratio);

   return sf::Color(red, green, blue, alpha);
}
