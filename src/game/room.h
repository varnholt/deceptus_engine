#pragma once

#include <optional>
#include <map>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "tmxparser/tmxobject.h"

struct Room
{
   Room() = default;
   Room(const sf::IntRect& rect);
   Room(const std::vector<sf::IntRect>& rect);

   static void deserialize(TmxObject* tmxObject, std::vector<Room>& rooms);
   static std::optional<Room> find(const sf::Vector2i& p, const std::vector<Room>& rooms);

   std::vector<sf::IntRect>::const_iterator findRect(const sf::Vector2i& p) const;
   std::optional<sf::Vector2i> correctedCamera(const sf::Vector2i& cameraCenter, const Room& activeRoom);

   std::vector<sf::IntRect> mRects;
   std::string mName;
};

