#pragma once

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <array>
#include <string>

#include "scriptproperty.h"

struct TmxObject;

struct Enemy
{
   Enemy() = default;

   void parse(TmxObject* object);
   void addPaths(const std::vector<std::vector<b2Vec2> >& paths);

   sf::Vector2i mPixelPosition;
   std::string mId;
   std::string mName;
   sf::IntRect mRect;
   std::array<sf::Vector2i, 4> mVertices;
   std::vector<b2Vec2> mPath;
   std::vector<int32_t> mPixelPath;
   bool mHasPath = false;
   std::vector<ScriptProperty> mProperties;
};

