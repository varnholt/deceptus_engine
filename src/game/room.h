#pragma once

#include <optional>
#include <map>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "tmxparser/tmxobject.h"

struct Room
{
   Room(const sf::FloatRect& rect);

   static void deserialize(TmxObject* tmxObject, std::vector<Room>& rooms);
   static std::optional<Room> find(const sf::Vector2f& p, const std::vector<Room>& rooms);

   std::vector<sf::FloatRect>::const_iterator findRect(const sf::Vector2f& p) const;
   void correctedCamera(float& x, float& y) const;
   std::optional<Room> computeCurrentRoom(const sf::Vector2f& cameraCenter, const std::vector<Room>& rooms) const;

   std::vector<sf::FloatRect> mRects;
   std::string mName;
   int32_t mId = 0;
};

