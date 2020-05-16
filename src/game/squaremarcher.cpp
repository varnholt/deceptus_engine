#include "squaremarcher.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

// #include <omp.h>

namespace {
 static const std::vector<std::string> verbs{
       "crashing", "landing", "communicating", "deserving", "squealing", "testing", "attaching", "flushing", "starting",
       "flowering", "telephoning", "stuffing", "tempting", "baring", "sacking", "disliking", "parking", "combing",
       "admiting", "facing", "passing", "replacing", "hoping", "fastening", "bleaching", "blinding", "switching",
       "welcoming", "ending", "preceding", "suiting", "terrifying", "crying", "wanting", "annoying", "touching",
       "peeling", "labelling", "beaming", "commanding", "wasting", "trembling", "pining", "confusing", "printing",
       "waiting", "fitting", "competing", "staying", "entertaining", "puncturing", "existing", "sealing", "dressing",
       "knocking", "breathing", "presenting", "offending", "mattering", "troting", "loving", "scratching", "carrying",
       "including", "squashing", "examining", "itching", "humming", "uniting", "changing", "sucking", "attending",
       "timing", "burying", "scraping", "whirling", "requesting", "launching", "munching", "flooding", "dragging",
       "watering", "camping", "debugging", "mashing", "detecting", "scaring", "chopping", "pinching", "noticing",
       "completing", "mending", "settling", "leveling", "skiing", "smashing", "silencing", "stamping", "flapping",
       "cleaning"
   };

   static const std::vector<std::string> nouns{
      "quilts", "slaves", "fuel", "gates", "earth", "lumber", "senses", "payments", "chickens", "offers", "wires",
      "roads", "crates", "signs", "tubs", "bears", "ice", "cats", "sisters", "bones", "butter", "sons", "quivers",
      "strings", "lakes", "bells", "sides", "tests", "fingers", "toys", "things", "sugar", "apparels", "girls", "boys",
      "lows", "flies", "respects", "forks", "talks", "crooks", "arguments", "sinks", "yams", "cactusses", "poisons",
      "tendencies", "pencils", "doctors", "runs", "verses", "milk", "turns", "hate", "legs", "giants", "experts",
      "cloth", "nations", "women", "bulbs", "companies", "shame", "berries", "offices", "machines", "temper", "quiet",
      "jail", "grain", "lamps", "mailboxes", "laborers", "pens", "cakes", "nuts", "snails", "pins", "summers", "self",
      "roses", "knees", "territories", "cherries", "coal", "furniture", "wings", "iron", "spaces", "marbles", "pets",
      "copper", "cans", "harmony", "events", "harbors", "crackers", "branches", "pockets", "dust"
   };
}


SquareMarcher::SquareMarcher(
   uint32_t w,
   uint32_t h,
   const std::vector<int32_t>& tiles,
   const std::vector<int32_t>& collidingTiles,
   const std::filesystem::path& cachePath,
   float scaleFactor
)
 : mWidth(w),
   mHeight(h),
   mTiles(tiles),
   mCollidingTiles(collidingTiles),
   mCachePath(cachePath),
   mScale(scaleFactor)
{
   // dumpMap();
   scan();
   optimize();
   scale();
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


void SquareMarcher::serialize()
{
   std::ofstream fileOut(mCachePath);
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


void SquareMarcher::deserialize()
{
   std::string line;

   std::ifstream fileIn(mCachePath);
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
   std::srand(static_cast<uint32_t>(std::time(nullptr)));

   std::ifstream fileIn(mCachePath);
   if (fileIn.fail())
   {
      // scan tiles until collision hit that wasn't visited
      for (auto y = 0u; y < mHeight; y++)
      {
         if ((y % 30) == 0) // 100 might do, too
         {
            std::cout << "[x] " << verbs[std::rand() % 100] << " " << nouns[std::rand() % 100] << std::endl;
            // std::cout << (y/static_cast<float>(mHeight)) * 100.0f << std::endl;
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

      serialize();
   }
   else
   {
      deserialize();
   }
}



void SquareMarcher::writeGridToImage(const std::filesystem::path& imagePath)
{
   std::ifstream fileIn(imagePath);
   if (fileIn.fail())
   {
      // float factor = 0.3333333333333f;
      float factor = 1.0f;
      sf::RenderTexture renderTexture;
      if (!renderTexture.create(
            static_cast<uint32_t>(mWidth * factor),
            static_cast<uint32_t>(mHeight * factor)
         )
      )
      {
          std::cout << "failed to create render texture" << std::endl;
          return;
      }

      renderTexture.clear();

      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::Red;
      quad[1].color = sf::Color::Red;
      quad[2].color = sf::Color::Red;
      quad[3].color = sf::Color::Red;

      for (auto y = 0u; y < mHeight; y++)
      {
         for (auto x = 0u; x < mWidth; x++)
         {
            if (isColliding(x, y))
            {
               quad[0].position = sf::Vector2f(static_cast<float>(x * factor),          static_cast<float>(y * factor));
               quad[1].position = sf::Vector2f(static_cast<float>(x * factor + factor), static_cast<float>(y * factor));
               quad[2].position = sf::Vector2f(static_cast<float>(x * factor + factor), static_cast<float>(y * factor + factor));
               quad[3].position = sf::Vector2f(static_cast<float>(x * factor),          static_cast<float>(y * factor + factor));

               renderTexture.draw(&quad[0], 4, sf::Quads);
            }
         }
      }

      renderTexture.display();

      // get the target texture (where the stuff has been drawn)
      const sf::Texture& texture = renderTexture.getTexture();
      texture.copyToImage().saveToFile(imagePath.string());
   }
}


void SquareMarcher::writePathToImage(const std::filesystem::path& imagePath)
{
   std::ifstream fileIn(imagePath);
   if (fileIn.fail())
   {
      const uint32_t factor = 1;
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
            sf::Vertex vertex;
            vertex.color = sf::Color::White;
            vertex.position.x = static_cast<float>(static_cast<uint32_t>(pos.x) * factor);
            vertex.position.y = static_cast<float>(static_cast<uint32_t>(pos.y) * factor);
            vertices.push_back(vertex);
         }

         vertices.push_back(vertices.at(0));
         renderTexture.draw(&vertices[0], vertices.size(), sf::LineStrip);
      }

      renderTexture.display();

      // get the target texture (where the stuff has been drawn)
      const sf::Texture& texture = renderTexture.getTexture();
      texture.copyToImage().saveToFile(imagePath.string());
   }
}


void SquareMarcher::optimize()
{
   //                     kick       kick
   //           ->         ->         ->         ->
   //      +----------+----------+----------+----------+
   //      |          |          |          |          |
   //      |          |          |          |          |
   //   /\ |          |          |          |          | \/
   //      |          |          |          |          |
   //      |          |          |          |          |
   //      +----------+----------+----------+----------+
   //           <-         <-         <-         <-
   //                     kick       kick

   // path is not suited to be optimized
   if (mPaths.empty() || mPaths.at(0).mDirs.empty())
   {
      return;
   }

   std::vector<Path> optimizedPaths;

   for (auto& path : mPaths)
   {
      Path optimized;

      if (path.mPolygon.size() < 5)
      {
         optimized = path;
      }
      else
      {
         for (auto i = 0u; i < path.mPolygon.size(); i++)
         {
            if (i == 0 || i == path.mPolygon.size() - 1)
            {
               optimized.mPolygon.push_back(path.mPolygon.at(i));
            }
            else
            {
               auto prevDir = path.mDirs[i - 1];
               auto currDir = path.mDirs[i    ];
               auto nextDir = path.mDirs[i + 1];

               if (!(prevDir == currDir && prevDir == nextDir))
               {
                  optimized.mPolygon.push_back(path.mPolygon.at(i));
               }
            }
         }
      }

      optimizedPaths.push_back(path);
   }

   mPaths = optimizedPaths;
}


void SquareMarcher::scale()
{
   for (auto& path : mPaths)
   {
      for (const auto& pos : path.mPolygon)
      {
         path.mScaled.push_back(
            sf::Vector2f{
               pos.x * mScale,
               pos.y * mScale
            }
         );
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
         path.mPolygon.push_back(
            sf::Vector2i(
               static_cast<int32_t>(mX),
               static_cast<int32_t>(mY)
            )
         );
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
