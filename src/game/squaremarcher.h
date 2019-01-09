#ifndef SQUAREMARCHER_H
#define SQUAREMARCHER_H

#include <map>
#include <vector>
#include <SFML/Graphics.hpp>

class SquareMarcher
{

public:

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
      std::vector<Direction> mDirs;

      void printPoly();
      void printDirs();
   };

   std::vector<Path> mPaths;


private:

   std::map<int, Direction> mMap;
   int32_t mWidth = 0;
   int32_t mHeight = 0;
   int32_t* mTiles = nullptr;
   std::vector<int32_t> mCollidingTiles;
   std::vector<int32_t> mVisited;

   int32_t mX = 0;
   int32_t mY = 0;
   Direction mDirCurrent = Direction::None;
   Direction mDirPrevious = Direction::None;

public:

  SquareMarcher(
    int32_t w,
    int32_t h,
    int32_t* tiles,
    const std::vector<int32_t>& collidingTiles
  );

  void printMap();


private:

   void scan();
   Path march(int32_t startX, int32_t startY);
   void updateDirection();
   void updatePosition();
   bool isColliding(int32_t x, int32_t y);
   bool isVisited(int32_t x, int32_t y);
};

#endif // SQUAREMARCHER_H
