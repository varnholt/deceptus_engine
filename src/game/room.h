#pragma once

#include <SFML/Graphics.hpp>

struct Room
{
   Room() = default;
   Room(const sf::IntRect& rect);

   sf::IntRect mRect;

};

