#include "squaremarcher.h"

#include <algorithm>


SquareMarcher::SquareMarcher(
  int32_t w,
  int32_t h,
  int32_t* tiles,
  const std::vector<int32_t>& collidingTiles
)
 : mWidth(w),
   mHeight(h),
   mTiles(tiles),
   mCollidingTiles(collidingTiles)
{
   scan();
}


void SquareMarcher::printMap()
{
   for (auto y = 0; y < mHeight; y++)
   {
      for (auto x = 0; x < mWidth; x++)
      {
         printf("%d", isColliding(x, y));
      }
      printf("\n");
   }
}


void SquareMarcher::scan()
{
   // scan tiles until collision hit that wasn't visited
   for (auto y = 0; y < mHeight; y++)
   {
      for (auto x = 0; x < mWidth; x++)
      {
         if (!isVisited(x, y) && isColliding(x, y))
         {
            auto p = march(x, y);

            if (!p.mPolygon.empty())
            {
               mPaths.push_back(p);
            }
         }
      }
   }
}


void SquareMarcher::updateDirection()
{
   auto fourPixels = 0;

   if (isColliding(mX - 1, mY - 1))
   {
      fourPixels |= static_cast<int32_t>(PixelLocation::TopLeft);
   }
   if (isColliding(mX, mY - 1))
   {
      fourPixels |= static_cast<int32_t>(PixelLocation::TopRight);
   }
   if (isColliding(mX - 1, mY))
   {
      fourPixels |= static_cast<int32_t>(PixelLocation::BottomLeft);
   }
   if (isColliding(mX, mY))
   {
      fourPixels |= static_cast<int32_t>(PixelLocation::BottomRight);
   }

   mDirPrevious = mDirCurrent;

   switch (fourPixels)
   {
      case 1:
      {
         mDirCurrent = Direction::Up;
         break;
      }
      case 2:
      {
         mDirCurrent = Direction::Right;
         break;
      }
      case 3:
      {
         mDirCurrent = Direction::Right;
         break;
      }
      case 4:
      {
         mDirCurrent = Direction::Left;
         break;
      }
      case 5:
      {
         mDirCurrent = Direction::Up;
         break;
      }
      case 6:
      {
         if (mDirPrevious == Direction::Up)
         {
            mDirCurrent = Direction::Left;
         }
         else
         {
            mDirCurrent = Direction::Right;
         }

         break;
      }
      case 7:
      {
         mDirCurrent = Direction::Right;
         break;
      }
      case 8:
      {
         mDirCurrent = Direction::Down;
         break;
      }
      case 9:
      {
         if (mDirPrevious == Direction::Right)
         {
            mDirCurrent = Direction::Up;
         }
         else
         {
            mDirCurrent = Direction::Down;
         }
         break;
      }
      case 10:
      {
         mDirCurrent = Direction::Down;
         break;
      }
      case 11:
      {
         mDirCurrent = Direction::Down;
         break;
      }
      case 12:
      {
         mDirCurrent = Direction::Left;
         break;
      }
      case 13:
      {
         mDirCurrent = Direction::Up;
         break;
      }
      case 14:
      {
         mDirCurrent = Direction::Left;
         break;
      }
      default:
      {
         mDirCurrent = Direction::None;
         break;
      }
   }
}


bool SquareMarcher::isColliding(int32_t x, int32_t y)
{
   if (x < 0 || x >= mWidth)
   {
      return false;
   }

   if (y < 0 || y >= mHeight)
   {
      return false;
   }

   auto val = mTiles[y * mWidth + x];
   return std::find(mCollidingTiles.begin(), mCollidingTiles.end(), val) != mCollidingTiles.end();
}


bool SquareMarcher::isVisited(int32_t x, int32_t y)
{
   if (x < 0 || x >= mWidth)
   {
      return false;
   }

   if (y < 0 || y >= mHeight)
   {
      return false;
   }

   auto val = y * mWidth + x;
   return std::find(mVisited.begin(), mVisited.end(), val) != mVisited.end();
}


void SquareMarcher::updatePosition()
{
   switch (mDirCurrent)
   {
      case Direction::Up:
         mY -= 1;
         break;

      case Direction::Down:
         mY += 1;
         break;

      case Direction::Left:
         mX -= 1;
         break;

      case Direction::Right:
         mX += 1;
         break;

      case Direction::None:
      default:
         break;
   }
}


SquareMarcher::Path SquareMarcher::march(int32_t startX, int32_t startY)
{
   mDirPrevious = Direction::None;

   mX = startX;
   mY = startY;

   Path path;

   while (true)
   {
      mVisited.push_back(mY * mWidth + mX);

      updateDirection();
      updatePosition();

      if (mDirCurrent != Direction::None)
      {
         path.mDirs.push_back(mDirCurrent);

         //if (mDirCurrent != mDirPrevious)
         {
            path.mPolygon.push_back(sf::Vector2i(mX, mY));
         }
      }

      if (mX == startX && mY == startY)
      {
         break;
      }
   }

   return path;
}


void SquareMarcher::Path::printPoly()
{
   printf("{ ");
   for (auto pos : mPolygon)
   {
      printf("{%d, %d}; ", pos.x, pos.y);
   }
   printf("}\n");
}


void SquareMarcher::Path::printDirs()
{
   for (auto dir : mDirs)
   {
      switch (dir)
      {
         case SquareMarcher::Direction::Up:
            printf("u;");
            break;
         case SquareMarcher::Direction::Down:
            printf("d;");
            break;
         case SquareMarcher::Direction::Left:
            printf("l;");
            break;
         case SquareMarcher::Direction::Right:
            printf("r;");
            break;
         default:
            break;
      }
   }
}
