#pragma once

#include "game/constants.h"
#include "level/tilemap.h"

#include <SFML/Graphics.hpp>
#include "box2d/box2d.h"

#include <cstdint>
#include <vector>

struct TmxLayer;
struct TmxTileSet;

/// \brief stores the atmosphere tile layer and supports tile lookup by world position.
struct Atmosphere
{
   /// \brief creates an empty atmosphere map.
   Atmosphere() = default;

   /// \brief releases cached atmosphere tile data.
   ~Atmosphere();

   /// \brief parses a TMX atmosphere layer into a relative tile id map.
   /// \param layer tile layer containing atmosphere tile gids.
   /// \param tileSet tileset used to convert global tile ids to local tile ids.
   void parse(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileSet);

   /// \brief returns the atmosphere tile at a world position given in box2d meters.
   /// \param pos_m world position in meters.
   /// \return atmosphere tile at the queried location, or AtmosphereTileInvalid when out of bounds.
   AtmosphereTile getTileForPosition(const b2Vec2& pos_m) const;

   /// \brief returns the atmosphere tile at a world position given in pixels.
   /// \param pos_px world position in pixels.
   /// \return atmosphere tile at the queried location, or AtmosphereTileInvalid when out of bounds.
   AtmosphereTile getTileForPosition(const sf::Vector2f& pos_px) const;

   /// \brief whether any atmosphere tile falls inside a rectangle given in pixels.
   /// \param rect_px region to test, in world pixels.
   /// \return true as soon as one atmosphere tile is found inside it.
   /// \note the distortion shader decides per fragment, from the atmosphere map at that fragment's
   ///       own position, so an atmosphere tile off screen cannot distort anything on it. With none
   ///       on screen the shader is an exact copy of what it samples, which is what lets the whole
   ///       atmosphere pass be skipped rather than run to no effect.
   bool hasTileInRect(const sf::FloatRect& rect_px) const;

   std::vector<int32_t> _map;

   int32_t _map_offset_x_px = 0;
   int32_t _map_offset_y_py = 0;
   int32_t _map_width_tl = 0;
   int32_t _map_height_tl = 0;

   std::vector<std::vector<sf::Vertex>> _outlines;
   std::shared_ptr<TileMap> _tile_map;
};
