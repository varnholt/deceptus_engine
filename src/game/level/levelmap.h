#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

/// \brief pixel art overview of a level, rasterized from its collision mesh.
///
/// the whole level is painted once at load time, exploration is not baked in. the map page
/// reveals the result piece by piece by drawing only the rectangles of the rooms the player
/// has already visited.
///
/// the map is stored as a pyramid of detail levels instead of a single texture. zooming picks a
/// different level rather than scaling one, so the wall outlines stay exactly one pixel wide at
/// every zoom step and can never be lost to filtering. each level keeps its walkable space (a one
/// tile corridor survives all the way to the coarsest level) and derives its own wall skin from it.
class LevelMap
{
public:
   /// \brief colors used while painting the map textures.
   struct Style
   {
      sf::Color _background{0, 0, 0, 0};     //!< everything that is neither walkable nor a wall skin
      sf::Color _interior{30, 44, 96, 255};  //!< walkable space
      sf::Color _wall{128, 176, 255, 255};   //!< solid cells that border walkable space
   };

   /// \brief one entry of the detail pyramid.
   struct DetailLevel
   {
      std::shared_ptr<sf::Texture> _texture;
      float _world_px_per_map_px = 8.0f;
   };

   /// \brief rasterizes a collision mesh into the map textures.
   /// \param obj_path wavefront obj holding the optimized level outlines in world pixels.
   /// \param width_tl level width in tiles.
   /// \param height_tl level height in tiles.
   /// \param tile_size_px edge length of one tile in world pixels.
   /// \return true when the mesh could be read and at least one texture was created.
   bool build(const std::filesystem::path& obj_path, int32_t width_tl, int32_t height_tl, int32_t tile_size_px);

   /// \brief returns whether usable map textures are available.
   /// \return true when build succeeded.
   bool isValid() const;

   /// \brief returns how many detail levels the pyramid has.
   /// \return detail level count, 0 when the map has not been built.
   size_t getDetailLevelCount() const;

   /// \brief returns the texture of one detail level.
   /// \param detail_level index into the detail pyramid, 0 is the most detailed.
   /// \return map texture, or nullptr when the index is out of range.
   const sf::Texture* getTexture(size_t detail_level) const;

   /// \brief returns the texture size of one detail level, in map pixels.
   /// \param detail_level index into the detail pyramid.
   /// \return texture size, or a zero vector when the index is out of range.
   sf::Vector2u getSize(size_t detail_level) const;

   /// \brief returns how many world pixels one map pixel of a detail level covers.
   /// \param detail_level index into the detail pyramid.
   /// \return world pixels per map pixel, or 0 when the index is out of range.
   float getWorldPixelsPerMapPixel(size_t detail_level) const;

   /// \brief converts a world position into map texture coordinates.
   /// \param position_px position in world pixels.
   /// \param detail_level index into the detail pyramid.
   /// \return position in map pixels.
   sf::Vector2f toMap(const sf::Vector2f& position_px, size_t detail_level) const;

   /// \brief converts a world rectangle into map texture coordinates.
   /// \param rect_px rectangle in world pixels.
   /// \param detail_level index into the detail pyramid.
   /// \return rectangle in map pixels.
   sf::FloatRect toMap(const sf::FloatRect& rect_px, size_t detail_level) const;

   Style _style;

private:
   /// \brief fills the base walkable grid from the mesh faces using the even-odd rule.
   /// \param obj_path wavefront obj holding the level outlines.
   /// \return true when the mesh contained geometry.
   bool rasterize(const std::filesystem::path& obj_path);

   /// \brief builds one detail level by merging blocks of base cells and painting the result.
   /// \param block_size how many base cells along each axis collapse into one map pixel.
   void addDetailLevel(int32_t block_size);

   std::vector<DetailLevel> _detail_levels;

   std::vector<bool> _interior;  //!< walkable cells at base resolution

   int32_t _base_width = 0;   //!< base map width in map pixels
   int32_t _base_height = 0;  //!< base map height in map pixels
   float _base_world_px_per_map_px = 8.0f;
};
