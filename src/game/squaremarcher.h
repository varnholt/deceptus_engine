#ifndef SQUAREMARCHER_H
#define SQUAREMARCHER_H

#include <filesystem>
#include <map>
#include <vector>
#include <SFML/Graphics.hpp>

class SquareMarcher
{

public:

   SquareMarcher(
      uint32_t w,
      uint32_t h,
      const std::vector<int32_t>& tiles,
      const std::vector<int32_t>& collidingTiles,
      const std::filesystem::path& cachePath,
      float scale = 1.0
   );

   void printMap();
   void dumpMap();

   void writeGridToImage(const std::filesystem::path& imagePath);
   void writePathToImage(const std::filesystem::path& imagePath);

   enum class Direction {
      None,
      Up,
      Down,
      Left,
      Right
   };

   enum class PixelLocation {
      None        = 0x00,
      TopLeft     = 0x01,
      TopRight    = 0x02,
      BottomLeft  = 0x04,
      BottomRight = 0x08
   };

   struct Path
   {
      std::vector<sf::Vector2i> mPolygon;
      std::vector<sf::Vector2f> mScaled;
      std::vector<Direction> mDirs;

      void printPoly();
      void printDirs();
   };

   std::vector<Path> mPaths;


private:

   void scan();
   Path march(uint32_t startX, uint32_t startY);
   void updateDirection();
   void updatePosition();
   bool isColliding(uint32_t x, uint32_t y);
   bool isVisited(uint32_t x, uint32_t y);
   void serialize();
   void deserialize();
   void optimize();
   void scale();


private:

   std::map<int, Direction> mMap;
   uint32_t mWidth = 0u;
   uint32_t mHeight = 0u;
   std::vector<int32_t> mTiles;
   std::vector<int32_t> mCollidingTiles;
   std::vector<bool> mVisited;
   std::filesystem::path mCachePath;

   uint32_t mX = 0;
   uint32_t mY = 0;
   Direction mDirCurrent = Direction::None;
   Direction mDirPrevious = Direction::None;
   float mScale = 1.0f;
};

#endif // SQUAREMARCHER_H
