#include "physics.h"

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "Box2D/Box2D.h"
#include "constants.h"
#include "meshtools.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxobjectgroup.h"
#include "tmxparser/tmxpolygon.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxtile.h"
#include "tmxparser/tmxtileset.h"



void Physics::parse(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath
)
{
   // std::cout << "parsing physics tiles vs. level layer (" << basePath.string() << ")" << std::endl;

   std::ifstream phsyicsFile(basePath / std::filesystem::path("physics_tiles.csv").string());

   std::map<int32_t, std::array<int32_t, 9>> map;
   std::string line;

   while (std::getline(phsyicsFile, line))
   {
      std::istringstream stream(line);
      std::string item;
      std::vector<int32_t> items;

      while (getline(stream, item, ','))
      {
         try
         {
            items.push_back(std::stoi(item));
         }
         catch (const std::invalid_argument& /*e*/)
         {
            // std::cerr << e.what() << std::endl;
         }
         catch (const std::out_of_range& /*e*/)
         {
            // std::cerr << e.what() << std::endl;
         }
      }

      if (items.size() < 10)
      {
         continue;
      }

      std::array<int32_t, 9> data;

      if (items.size() == 11) // a range: 0,12,1,1,1,1,1,1,1,1,1
      {
         std::copy_n(items.begin() + 2, 9, data.begin());
         for (auto key = items[0]; key <= items[1]; key++)
         {
            map[key] = data;
         }
      }
      else if (items.size() == 10) // a single entry: 64,3,3,3,3,0,0,3,0,0
      {
         std::copy_n(items.begin() + 1, 9, data.begin());
         map[items[0]] = data;
      }

      // for (auto x : data)
      // {
      //    std::cout << x << ",";
      // }
      // std::cout << std::endl;
   }

   mGridWidth  = layer->mWidth  * 3;
   mGridHeight = layer->mHeight * 3;
   mGridSize   = mGridWidth * mGridHeight;

   // a larger grid and copy tile contents in there
   mPhysicsMap.resize(mGridSize);

   auto yi = 0u;
   for (auto y = 0u; y < layer->mHeight; y++)
   {
      for (auto x = 0u; x < layer->mWidth; x++)
      {
         const auto key = layer->mData[y * layer->mWidth + x];

         if (key != 0)
         {
            const auto it = map.find(key - tileSet->mFirstGid);

            // std::cout << key << ",";

            if (it != map.end())
            {
               const auto& arr = (*it).second;

               const auto row1 = ((y + yi + 0) * mGridWidth) + (x * 3);
               const auto row2 = ((y + yi + 1) * mGridWidth) + (x * 3);
               const auto row3 = ((y + yi + 2) * mGridWidth) + (x * 3);

               for (auto xi = 0u; xi < 3; xi++) mPhysicsMap[row1 + xi] = arr[xi + 0];
               for (auto xi = 0u; xi < 3; xi++) mPhysicsMap[row2 + xi] = arr[xi + 3];
               for (auto xi = 0u; xi < 3; xi++) mPhysicsMap[row3 + xi] = arr[xi + 6];
            }
         }
      }

      yi += 2;

      // std::cout << std::endl;
   }
}


//-----------------------------------------------------------------------------
void Physics::parseCollidingTiles(TmxLayer* layer, TmxTileSet* tileSet)
{
   const auto tiles  = layer->mData;
   const auto width  = layer->mWidth;
   const auto height = layer->mHeight;
   const auto offsetX = layer->mOffsetX;
   const auto offsetY = layer->mOffsetY;

   if (tileSet == nullptr)
   {
      // std::cout << "tileset is a nullptr" << std::endl;
      return;
   }

   const auto tileMap = tileSet->mTileMap;

   std::vector<b2Vec2> vertices;
   std::vector<std::vector<uint32_t>> faces;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         const auto tileNumber = tiles[y * width + x];
         auto tileRelative = -1;

         if (tileNumber != 0)
         {
            tileRelative = tileNumber - tileSet->mFirstGid;
            auto tileIt = tileMap.find(tileRelative);

            if (tileIt != tileMap.end())
            {
               auto tile = tileIt->second;
               auto objects = tile->mObjectGroup;

               if (objects)
               {
                  for (auto it : objects->mObjects)
                  {
                     auto object = it.second;

                     auto poly = object->mPolygon;
                     auto line = object->mPolyLine;

                     std::vector<sf::Vector2f> points;

                     if (poly)
                     {
                        points = poly->mPolyLine;
                     }
                     else if (line)
                     {
                        points = line->mPolyLine;
                     }
                     else
                     {
                        auto x = object->mX;
                        auto y = object->mY;
                        auto w = object->mWidth;
                        auto h = object->mHeight;

                        points = {
                           {x,     y    },
                           {x,     y + h},
                           {x + w, y + h},
                           {x + w, y    },
                        };
                     }

                     if (!points.empty())
                     {
                        std::vector<uint32_t> face;
                        for (const auto& p : points)
                        {
                           const auto px = (offsetX + static_cast<int32_t>(x)) * PIXELS_PER_TILE + p.x;
                           const auto py = (offsetY + static_cast<int32_t>(y)) * PIXELS_PER_TILE + p.y;
                           const auto v = b2Vec2(px, py);

                           const auto& it = std::find_if(vertices.begin(), vertices.end(), [v](const b2Vec2& other){
                              return (
                                    fabs(v.x - other.x) < 0.001f
                                 && fabs(v.y - other.y) < 0.001f
                              );
                           });

                           auto vertexId = 0u;
                           if (it == vertices.end())
                           {
                              vertexId = static_cast<uint32_t>(vertices.size());
                              vertices.push_back(v);
                           }
                           else
                           {
                              vertexId = static_cast<uint32_t>(it - vertices.begin());
                           }

                           face.push_back(vertexId + 1); // wavefront obj starts indexing at 1
                        }

                        faces.push_back(face);
                     }
                  }
               }
            }
         }
      }
   }

   // https://en.wikipedia.org/wiki/Wavefront_.obj_file
   Mesh::writeObj("layer_" + layer->mName + ".obj", vertices, faces);

   // weld all the things
   //
   // concept:
   //
   //    If one of the vertices of the neighboring tile touches the current vector (is within epsilon reach)
   //    the follow that path.
   //
   //    If the path can go left or right, always choose the right path.
   //
   //                           *-- -  -
   //     1         23          |
   //     *---------**----------* 4
   //     |         ||          |
   //     *---------**----------* 5
   //     8         76
   //
   //    Create a new path based on the traced vertices. Since a lot of old edges will become obsolete
   //    edges: 1-3, 3-4, 4-5, 5-7, 7-8, 8-1
   //
   //    After creating a closed path, delete all vertices with almost identical x or y positions
   //    edges: 1-4, 4-5, 5-8, 8-1
}
