#include "fakesfml.h"

sf::Vector2f::Vector2f(float x, float y) : x(x), y(y)
{
}

sf::Vector2f sf::Vector2f::operator+=(const Vector2f& other)
{
   return sf::Vector2f{x + other.x, y + other.y};
}

sf::Vector2f sf::Vector2f::operator+(const Vector2f& other)
{
   return sf::Vector2f{x + other.x, y + other.y};
}

sf::Vector2f sf::Vector2f::operator*(float val)
{
   return sf::Vector2f{x * val, y * val};
}
