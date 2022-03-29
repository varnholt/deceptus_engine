#pragma once

#include "constants.h"
#include "tilemap.h"

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <cstdint>
#include <vector>

struct TmxLayer;
struct TmxTileSet;

struct Atmosphere
{
   Atmosphere() = default;
   ~Atmosphere();

   void parse(TmxLayer* layer, TmxTileSet* tileSet);
   AtmosphereTile getTileForPosition(const b2Vec2& playerPos) const;

   std::vector<int32_t> _map;

   int32_t _map_offset_x_px = 0;
   int32_t _map_offset_y_py = 0;
   uint32_t _map_width_tl = 0;
   uint32_t _map_height_tl = 0;

   std::vector<std::vector<sf::Vertex>> _outlines;
   std::shared_ptr<TileMap> _tile_map;
};

