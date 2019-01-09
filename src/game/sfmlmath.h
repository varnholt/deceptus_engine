#ifndef SFMLMATH_H
#define SFMLMATH_H

#include <SFML/Graphics.hpp>

class SfmlMath
{
public:
   SfmlMath();

   static float length(const sf::Vector2f&);
   static float lengthSquared(const sf::Vector2f&);
   static sf::Vector2f normalize(const sf::Vector2f& v);
};

#endif // SFMLMATH_H
