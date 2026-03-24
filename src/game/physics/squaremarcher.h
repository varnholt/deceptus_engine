#ifndef SQUAREMARCHER_H
#define SQUAREMARCHER_H

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <map>
#include <vector>

/// \brief extracts collision outlines from a tile grid using a marching-squares style contour walk.
class SquareMarcher
{
public:
   /// \brief builds contour paths from tile collision data and prepares scaled polygon output.
   /// \param w map width in tiles.
   /// \param h map height in tiles.
   /// \param tiles tile ids for the full grid.
   /// \param collidingTiles tile ids considered solid during contour extraction.
   /// \param cachePath file path used to cache or load traced polygon paths.
   /// \param scale scale factor applied when generating floating-point path coordinates.
   SquareMarcher(
      uint32_t w,
      uint32_t h,
      const std::vector<int32_t>& tiles,
      const std::vector<int32_t>& collidingTiles,
      const std::filesystem::path& cachePath,
      float scale = 1.0
   );

   /// \brief prints a binary collision map to stdout for debugging.
   void printMap();

   /// \brief writes the raw tile id map to map.dump for debugging.
   void dumpMap();

   /// \brief renders colliding grid cells to an image file when the target file does not exist.
   /// \param imagePath destination image path.
   void writeGridToImage(const std::filesystem::path& imagePath);

   /// \brief renders extracted polygon outlines to an image file when the target file does not exist.
   /// \param imagePath destination image path.
   void writePathToImage(const std::filesystem::path& imagePath);

   enum class Direction
   {
      None,
      Up,
      Down,
      Left,
      Right
   };

   enum class PixelLocation
   {
      None = 0x00,
      TopLeft = 0x01,
      TopRight = 0x02,
      BottomLeft = 0x04,
      BottomRight = 0x08
   };

   /// \brief stores one traced contour with integer points, scaled points, and step directions.
   struct Path
   {
      std::vector<sf::Vector2i> _polygon;
      std::vector<sf::Vector2f> _scaled;
      std::vector<Direction> _dirs;

      /// \brief prints polygon vertices for debugging.
      void printPoly();

      /// \brief prints encoded march directions for debugging.
      void printDirs();
   };

   std::vector<Path> _paths;

private:
   /// \brief loads cached contours or scans the tile grid to generate and cache new contours.
   void scan();

   /// \brief traces one closed contour starting at a colliding unvisited tile coordinate.
   /// \param startX starting x coordinate in tile space.
   /// \param startY starting y coordinate in tile space.
   /// \return traced contour path including polygon points and step directions.
   Path march(uint32_t startX, uint32_t startY);

   /// \brief computes the next march direction from the current 2x2 occupancy configuration.
   void updateDirection();

   /// \brief advances the current march position by one step in the current direction.
   void updatePosition();

   /// \brief checks whether a tile coordinate is inside bounds and marked as colliding.
   /// \param x x coordinate in tile space.
   /// \param y y coordinate in tile space.
   /// \return true when the coordinate maps to a colliding tile id.
   bool isColliding(uint32_t x, uint32_t y);

   /// \brief checks whether a tile coordinate has already been visited by contour tracing.
   /// \param x x coordinate in tile space.
   /// \param y y coordinate in tile space.
   /// \return true when the coordinate is within bounds and marked as visited.
   bool isVisited(uint32_t x, uint32_t y);

   /// \brief writes traced polygon paths to the cache file.
   void serialize();

   /// \brief reads polygon paths from the cache file.
   void deserialize();

   /// \brief removes redundant collinear points from traced paths.
   void optimize();

   /// \brief fills scaled floating-point coordinates for each path using the configured scale factor.
   void scale();

private:
   std::map<int, Direction> _map;
   uint32_t _width = 0u;
   uint32_t _height = 0u;
   std::vector<int32_t> _tiles;
   std::vector<int32_t> _colliding_tiles;
   std::vector<bool> _visited;
   std::filesystem::path _cache_path;

   uint32_t _x = 0;
   uint32_t _y = 0;
   Direction _dir_current = Direction::None;
   Direction _dir_previous = Direction::None;
   float _scale = 1.0f;
};

#endif  // SQUAREMARCHER_H
