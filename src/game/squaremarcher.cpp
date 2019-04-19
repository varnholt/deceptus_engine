#include "squaremarcher.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>


SquareMarcher::SquareMarcher(
    uint32_t w,
    uint32_t h,
    const std::vector<int32_t>& tiles,
    const std::vector<int32_t>& collidingTiles
)
 : mWidth(w),
   mHeight(h),
   mTiles(tiles),
   mCollidingTiles(collidingTiles)
{
   dumpMap();
   scan();
}


void SquareMarcher::printMap()
{
   for (auto y = 0u; y < mHeight; y++)
   {
      for (auto x = 0u; x < mWidth; x++)
      {
         printf("%d", isColliding(x, y));
      }
      printf("\n");
   }
}


void SquareMarcher::dumpMap()
{
   std::ofstream fileOut("map.dump");
   for (auto y = 0u; y < mHeight; y++)
   {
      for (auto x = 0u; x < mWidth; x++)
      {
         fileOut << mTiles[y * mWidth + x];
      }
      fileOut << std::endl;
   }
   fileOut.close();
}


void SquareMarcher::serialize(const std::string& filename)
{
   std::ofstream fileOut(filename);
   for (const auto& path : mPaths)
   {
      for (const auto& pos : path.mPolygon)
      {
         fileOut << std::fixed << std::setprecision(8) << pos.x;
         fileOut << ",";
         fileOut << std::fixed << std::setprecision(3) << pos.y;
         fileOut << ";";
      }

      fileOut << std::endl;
   }
   fileOut.close();
}

void SquareMarcher::deserialize(const std::string& filename)
{
   std::string line;

   std::ifstream fileIn(filename);
   while (std::getline(fileIn, line))
   {
      std::istringstream lineStream(line);
      std::string item;

      std::string eatComma;
      Path path;

      while (getline(lineStream, item, ';'))
      {
         std::istringstream posStream(item);
         auto x = 0;
         auto y = 0;

         posStream >> x;
         std::getline(posStream, eatComma, ',');
         posStream >> y;

         path.mPolygon.push_back(sf::Vector2i{x, y});
      }

      mPaths.push_back(path);
   }
}


void SquareMarcher::scan()
{
   std::string filename = "physics.paths";
   std::ifstream fileIn(filename);
   if (fileIn.fail())
   {
      // scan tiles until collision hit that wasn't visited
      for (auto y = 0u; y < mHeight; y++)
      {
         if ((y % 10) == 0)
         {
            std::cout << (y/static_cast<float>(mHeight)) * 100.0f << std::endl;
         }

         for (auto x = 0u; x < mWidth; x++)
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

      serialize(filename);
   }
   else
   {
      deserialize(filename);
      debugPaths();
   }
}


void SquareMarcher::debugPaths()
{
   uint32_t factor = 5;
   sf::RenderTexture renderTexture;
   if (!renderTexture.create(mWidth * factor, mHeight * factor))
   {
       std::cout << "failed to create render texture" << std::endl;
       return;
   }

   renderTexture.clear();

   for (const auto& path : mPaths)
   {
      std::vector<sf::Vertex> vertices;
      for (const auto& pos : path.mPolygon)
      {
         vertices.push_back(
            sf::Vector2f{
               static_cast<float>(pos.x * factor),
               static_cast<float>(pos.y * factor)
            }
         );
      }
      vertices.push_back(vertices.at(0));

      renderTexture.draw(&vertices[0], vertices.size(), sf::LineStrip);
   }

   renderTexture.display();

   // get the target texture (where the stuff has been drawn)
   const sf::Texture& texture = renderTexture.getTexture();
   texture.copyToImage().saveToFile("paths.png");
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


bool SquareMarcher::isColliding(uint32_t x, uint32_t y)
{
   if (x >= mWidth)
   {
      return false;
   }

   if (y >= mHeight)
   {
      return false;
   }

   auto val = mTiles[y * mWidth + x];
   return std::find(mCollidingTiles.begin(), mCollidingTiles.end(), val) != mCollidingTiles.end();
}


bool SquareMarcher::isVisited(uint32_t x, uint32_t y)
{
   if (x >= mWidth)
   {
      return false;
   }

   if (y >= mHeight)
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
         break;
   }
}


SquareMarcher::Path SquareMarcher::march(uint32_t startX, uint32_t startY)
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
