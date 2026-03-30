#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct TmxLayer;
struct TmxTileSet;

/// \brief builds collision lookup data and exports tile collision geometry from TMX layers.
struct Physics
{
   /// \brief loads physics tile patterns and expands each tile into a 3x3 collision grid.
   /// \param layer tile layer containing map tile ids.
   /// \param tileSet tileset used to resolve tile id offsets.
   /// \param basePath base directory that contains physics_tiles.csv.
   void parse(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileSet, const std::filesystem::path& basePath);

   /// \brief exports polygon and polyline collision objects from the layer into a wavefront obj mesh.
   /// \param layer tile layer whose tile objects are converted into mesh faces.
   /// \param tileSet tileset providing per-tile object groups.
   /// \param path output path for the generated obj file.
   /// \return true when at least one vertex was exported and the obj file was written.
   bool dumpObj(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileSet, const std::filesystem::path& path);

   int32_t _grid_width = 0;
   int32_t _grid_height = 0;
   int32_t _grid_size = 0;

   std::vector<int32_t> _physics_map;
};
