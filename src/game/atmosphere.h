#pragma once

#include "constants.h"
#include "tilemap.h"

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <cstdint>
#include <vector>

struct Atmosphere
{
   Atmosphere() = default;

   std::vector<int32_t> mMap;

   int32_t mMapOffsetX = 0;
   int32_t mMapOffsetY = 0;
   uint32_t mMapWidth = 0;
   uint32_t mMapHeight = 0;

   std::vector<std::vector<sf::Vertex>> mOutlines;
   std::vector<std::vector<b2Vec2>> mChains;
   std::shared_ptr<TileMap> mTileMap;

   AtmosphereTile getTileForPosition(const b2Vec2& playerPos) const;
};

