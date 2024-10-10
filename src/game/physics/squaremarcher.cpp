#include "squaremarcher.h"

#include "framework/tools/log.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

SquareMarcher::SquareMarcher(
   uint32_t w,
   uint32_t h,
   const std::vector<int32_t>& tiles,
   const std::vector<int32_t>& colliding_tiles,
   const std::filesystem::path& cache_path,
   float scaleFactor
)
    : _width(w), _height(h), _tiles(tiles), _colliding_tiles(colliding_tiles), _cache_path(cache_path), _scale(scaleFactor)
{
   _visited.resize(_width * _height);

   // dumpMap();
   scan();
   optimize();
   scale();
}

void SquareMarcher::printMap()
{
   for (auto y = 0u; y < _height; y++)
   {
      for (auto x = 0u; x < _width; x++)
      {
         std::cout << std::format("{:d}", isColliding(x, y));
      }
      std::cout << '\n';
   }
}

void SquareMarcher::dumpMap()
{
   std::ofstream fileOut("map.dump");
   for (auto y = 0u; y < _height; y++)
   {
      for (auto x = 0u; x < _width; x++)
      {
         fileOut << _tiles[y * _width + x];
      }
      fileOut << std::endl;
   }
   fileOut.close();
}

void SquareMarcher::serialize()
{
   std::ofstream file_out(_cache_path);
   for (const auto& path : _paths)
   {
      for (const auto& pos : path._polygon)
      {
         file_out << std::fixed << std::setprecision(8) << pos.x;
         file_out << ",";
         file_out << std::fixed << std::setprecision(3) << pos.y;
         file_out << ";";
      }

      file_out << std::endl;
   }
   file_out.close();
}

void SquareMarcher::deserialize()
{
   std::string line;

   std::ifstream file_in(_cache_path);
   while (std::getline(file_in, line))
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

         path._polygon.emplace_back(x, y);
      }

      _paths.push_back(path);
   }
}

void SquareMarcher::scan()
{
   std::srand(static_cast<uint32_t>(std::time(nullptr)));

   std::ifstream fileIn(_cache_path);
   if (fileIn.fail())
   {
      // scan tiles until collision hit that wasn't visited
      for (auto y = 0u; y < _height; y++)
      {
         for (auto x = 0u; x < _width; x++)
         {
            if (!isVisited(x, y) && isColliding(x, y))
            {
               auto p = march(x, y);

               if (!p._polygon.empty())
               {
                  _paths.push_back(p);
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

void SquareMarcher::writeGridToImage(const std::filesystem::path& image_path)
{
   std::ifstream file_in(image_path);
   if (!file_in.fail())
   {
      return;
   }

   float factor = 1.0f;
   sf::RenderTexture render_texture;
   if (!render_texture.create(static_cast<uint32_t>(_width * factor), static_cast<uint32_t>(_height * factor)))
   {
      Log::Error() << "failed to create render texture";
      return;
   }

   render_texture.clear();

   sf::VertexArray quad(sf::Quads, 4);
   quad[0].color = sf::Color::Red;
   quad[1].color = sf::Color::Red;
   quad[2].color = sf::Color::Red;
   quad[3].color = sf::Color::Red;

   for (auto y = 0u; y < _height; y++)
   {
      for (auto x = 0u; x < _width; x++)
      {
         if (isColliding(x, y))
         {
            quad[0].position = sf::Vector2f(static_cast<float>(x * factor), static_cast<float>(y * factor));
            quad[1].position = sf::Vector2f(static_cast<float>(x * factor + factor), static_cast<float>(y * factor));
            quad[2].position = sf::Vector2f(static_cast<float>(x * factor + factor), static_cast<float>(y * factor + factor));
            quad[3].position = sf::Vector2f(static_cast<float>(x * factor), static_cast<float>(y * factor + factor));

            render_texture.draw(&quad[0], 4, sf::Quads);
         }
      }
   }

   render_texture.display();

   // get the target texture (where the stuff has been drawn)
   const sf::Texture& texture = render_texture.getTexture();
   texture.copyToImage().saveToFile(image_path.string());
}

void SquareMarcher::writePathToImage(const std::filesystem::path& image_path)
{
   std::ifstream file_in(image_path);
   if (!file_in.fail())
   {
      return;
   }

   const uint32_t factor = 1;
   sf::RenderTexture render_texture;
   if (!render_texture.create(_width * factor, _height * factor))
   {
      Log::Error() << "failed to create render texture";
      return;
   }

   render_texture.clear();

   for (const auto& path : _paths)
   {
      std::vector<sf::Vertex> vertices;
      for (const auto& pos : path._polygon)
      {
         sf::Vertex vertex;
         vertex.color = sf::Color::White;
         vertex.position.x = static_cast<float>(static_cast<uint32_t>(pos.x) * factor);
         vertex.position.y = static_cast<float>(static_cast<uint32_t>(pos.y) * factor);
         vertices.push_back(vertex);
      }

      vertices.push_back(vertices.at(0));
      render_texture.draw(&vertices[0], vertices.size(), sf::LineStrip);
   }

   render_texture.display();

   // get the target texture (where the stuff has been drawn)
   const sf::Texture& texture = render_texture.getTexture();
   texture.copyToImage().saveToFile(image_path.string());
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
   if (_paths.empty() || _paths.at(0)._dirs.empty())
   {
      return;
   }

   std::vector<Path> optimized_paths;

   for (auto& path : _paths)
   {
      Path optimized;

      if (path._polygon.size() < 5)
      {
         optimized = path;
      }
      else
      {
         for (auto i = 0u; i < path._polygon.size(); i++)
         {
            if (i == 0 || i == path._polygon.size() - 1)
            {
               optimized._polygon.push_back(path._polygon.at(i));
            }
            else
            {
               auto prevDir = path._dirs[i - 1];
               auto currDir = path._dirs[i];
               auto nextDir = path._dirs[i + 1];

               if (!(prevDir == currDir && prevDir == nextDir))
               {
                  optimized._polygon.push_back(path._polygon.at(i));
               }
            }
         }
      }

      optimized_paths.push_back(optimized);
   }

   _paths = optimized_paths;
}

void SquareMarcher::scale()
{
   for (auto& path : _paths)
   {
      for (const auto& pos : path._polygon)
      {
         path._scaled.emplace_back(pos.x * _scale, pos.y * _scale);
      }
   }
}

void SquareMarcher::updateDirection()
{
   auto four_pixels = 0;

   if (isColliding(_x - 1, _y - 1))
   {
      four_pixels |= static_cast<int32_t>(PixelLocation::TopLeft);
   }
   if (isColliding(_x, _y - 1))
   {
      four_pixels |= static_cast<int32_t>(PixelLocation::TopRight);
   }
   if (isColliding(_x - 1, _y))
   {
      four_pixels |= static_cast<int32_t>(PixelLocation::BottomLeft);
   }
   if (isColliding(_x, _y))
   {
      four_pixels |= static_cast<int32_t>(PixelLocation::BottomRight);
   }

   _dir_previous = _dir_current;

   switch (four_pixels)
   {
      case 1:
      {
         _dir_current = Direction::Up;
         break;
      }
      case 2:
      {
         _dir_current = Direction::Right;
         break;
      }
      case 3:
      {
         _dir_current = Direction::Right;
         break;
      }
      case 4:
      {
         _dir_current = Direction::Left;
         break;
      }
      case 5:
      {
         _dir_current = Direction::Up;
         break;
      }
      case 6:
      {
         if (_dir_previous == Direction::Up)
         {
            _dir_current = Direction::Left;
         }
         else
         {
            _dir_current = Direction::Right;
         }

         break;
      }
      case 7:
      {
         _dir_current = Direction::Right;
         break;
      }
      case 8:
      {
         _dir_current = Direction::Down;
         break;
      }
      case 9:
      {
         if (_dir_previous == Direction::Right)
         {
            _dir_current = Direction::Up;
         }
         else
         {
            _dir_current = Direction::Down;
         }
         break;
      }
      case 10:
      {
         _dir_current = Direction::Down;
         break;
      }
      case 11:
      {
         _dir_current = Direction::Down;
         break;
      }
      case 12:
      {
         _dir_current = Direction::Left;
         break;
      }
      case 13:
      {
         _dir_current = Direction::Up;
         break;
      }
      case 14:
      {
         _dir_current = Direction::Left;
         break;
      }
      default:
      {
         _dir_current = Direction::None;
         break;
      }
   }
}

bool SquareMarcher::isColliding(uint32_t x, uint32_t y)
{
   if (x >= _width)
   {
      return false;
   }

   if (y >= _height)
   {
      return false;
   }

   auto val = _tiles[y * _width + x];
   return std::find(_colliding_tiles.begin(), _colliding_tiles.end(), val) != _colliding_tiles.end();
}

bool SquareMarcher::isVisited(uint32_t x, uint32_t y)
{
   if (x >= _width)
   {
      return false;
   }

   if (y >= _height)
   {
      return false;
   }

   auto key = y * _width + x;
   return _visited[key];
}

void SquareMarcher::updatePosition()
{
   switch (_dir_current)
   {
      case Direction::Up:
         _y -= 1;
         break;

      case Direction::Down:
         _y += 1;
         break;

      case Direction::Left:
         _x -= 1;
         break;

      case Direction::Right:
         _x += 1;
         break;

      case Direction::None:
         break;
   }
}

SquareMarcher::Path SquareMarcher::march(uint32_t start_x, uint32_t start_y)
{
   _dir_previous = Direction::None;

   _x = start_x;
   _y = start_y;

   Path path;

   while (true)
   {
      _visited[_y * _width + _x] = true;

      updateDirection();
      updatePosition();

      if (_dir_current != Direction::None)
      {
         path._dirs.push_back(_dir_current);
         path._polygon.emplace_back(
            static_cast<int32_t>(_x), static_cast<int32_t>(_y)

         );
      }

      if (_x == start_x && _y == start_y)
      {
         break;
      }
   }

   return path;
}

void SquareMarcher::Path::printPoly()
{
   std::cout << "{ ";
   for (const auto& pos : _polygon)
   {
      std::cout << std::format("{{{}, {}}}; ", pos.x, pos.y);
   }
   std::cout << "}}\n";
}

void SquareMarcher::Path::printDirs()
{
   for (const auto& dir : _dirs)
   {
      switch (dir)
      {
         case SquareMarcher::Direction::Up:
            std::cout << "u;";
            break;
         case SquareMarcher::Direction::Down:
            std::cout << "d;";
            break;
         case SquareMarcher::Direction::Left:
            std::cout << "l;";
            break;
         case SquareMarcher::Direction::Right:
            std::cout << "r;";
            break;
         default:
            break;
      }
   }
   std::cout << '\n';
}
