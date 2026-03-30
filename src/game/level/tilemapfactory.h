#pragma once

struct TmxLayer;

#include <memory>
#include <vector>
#include "tilemap.h"

namespace TileMapFactory
{
/// \brief creates a tilemap instance for a TMX layer.
/// \details returns StencilTileMap when the layer declares a stencil_reference property, otherwise TileMap.
/// \param layer TMX layer description.
/// \return newly created tilemap implementation for the layer.
std::shared_ptr<TileMap> makeTileMap(const std::shared_ptr<TmxLayer>& layer);

/// \brief resolves stencil references across loaded tilemaps.
/// \param tile_maps complete tilemap list for the level.
void merge(const std::vector<std::shared_ptr<TileMap>>& tile_maps);
}  // namespace TileMapFactory
