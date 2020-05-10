#pragma once

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <array>
#include <string>

struct TmxObject;

struct Enemy
{
   Enemy() = default;

   void parse(TmxObject* object);
   void addChain(const std::vector<std::vector<b2Vec2> >& chains);

   std::string mId;
   std::string mName;
   sf::IntRect mRect;
   std::array<sf::Vector2i, 4> mVertices;
   std::vector<b2Vec2> mChain;
   std::vector<int32_t> mPixelChain;
   // std::vector<sf::Vector2f> mPixelChain;
};

