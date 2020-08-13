#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

struct Room
{
   Room() = default;
   Room(const std::vector<sf::IntRect>& rect);

   bool contains(const sf::Vector2i& p) const;
   void correctCamera(const sf::Vector2i& cameraCenter, const Room& activeRoom);

   std::vector<sf::IntRect> mRects;
};

