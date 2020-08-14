#pragma once

#include <optional>
#include <vector>
#include <SFML/Graphics.hpp>

struct Room
{
   Room() = default;
   Room(const std::vector<sf::IntRect>& rect);

   std::vector<sf::IntRect>::const_iterator findRect(const sf::Vector2i& p) const;
   std::optional<sf::Vector2i> correctedCamera(const sf::Vector2i& cameraCenter, const Room& activeRoom);

   std::vector<sf::IntRect> mRects;
};

