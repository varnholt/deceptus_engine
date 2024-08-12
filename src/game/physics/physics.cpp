#include "physics.h"

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include <box2d/box2d.h>

#include "constants.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxtile.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "game/io/meshtools.h"

namespace
{
uint64_t computeHash(float px, float py)
{
   struct VectorHash
   {
      VectorHash(float x, float y) : x(x), y(y)
      {
      }

      float x;
      float y;
   };

   VectorHash hv(px, py);
   return *reinterpret_cast<uint64_t*>(&hv);
}

}  // namespace

void Physics::parse(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileSet,
   const std::filesystem::path& base_path
)
{
   // Log::Info() << "parsing physics tiles vs. level layer (" << basePath.string() << ")";

   std::ifstream phsyicsFile(base_path / std::filesystem::path("physics_tiles.csv").string());

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
            // Log::Error() << e.what();
         }
         catch (const std::out_of_range& /*e*/)
         {
            // Log::Error() << e.what();
         }
      }

      if (items.size() < 10)
      {
         continue;
      }

      std::array<int32_t, 9> data;

      if (items.size() == 11)  // a range: 0,12,1,1,1,1,1,1,1,1,1
      {
         std::copy_n(items.begin() + 2, 9, data.begin());
         for (auto key = items[0]; key <= items[1]; key++)
         {
            map[key] = data;
         }
      }
      else if (items.size() == 10)  // a single entry: 64,3,3,3,3,0,0,3,0,0
      {
         std::copy_n(items.begin() + 1, 9, data.begin());
         map[items[0]] = data;
      }
   }

   _grid_width = layer->_width_tl * 3;
   _grid_height = layer->_height_tl * 3;
   _grid_size = _grid_width * _grid_height;

   // a larger grid and copy tile contents in there
   _physics_map.resize(_grid_size);

   auto yi = 0u;
   for (auto y = 0u; y < layer->_height_tl; y++)
   {
      for (auto x = 0u; x < layer->_width_tl; x++)
      {
         const auto key = layer->_data[y * layer->_width_tl + x];

         if (key != 0)
         {
            const auto it = map.find(key - tileSet->_first_gid);

            if (it != map.end())
            {
               const auto& arr = (*it).second;

               const auto row1 = ((y + yi + 0) * _grid_width) + (x * 3);
               const auto row2 = ((y + yi + 1) * _grid_width) + (x * 3);
               const auto row3 = ((y + yi + 2) * _grid_width) + (x * 3);

               for (auto xi = 0u; xi < 3; xi++)
               {
                  _physics_map[row1 + xi] = arr[xi + 0];
               }
               for (auto xi = 0u; xi < 3; xi++)
               {
                  _physics_map[row2 + xi] = arr[xi + 3];
               }
               for (auto xi = 0u; xi < 3; xi++)
               {
                  _physics_map[row3 + xi] = arr[xi + 6];
               }
            }
         }
      }

      yi += 2;
   }
}

//-----------------------------------------------------------------------------
bool Physics::dumpObj(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset, const std::filesystem::path& path)
{
   if (!tileset)
   {
      // Log::Error() << "tileset is a nullptr";
      return false;
   }

   std::unordered_map<uint64_t, uint32_t> face_indices;

   const auto& tiles = layer->_data;
   const auto width_tl = layer->_width_tl;
   const auto height_tl = layer->_height_tl;
   const auto offset_x_px = layer->_offset_x_px;
   const auto offset_y_px = layer->_offset_y_px;
   const auto& tile_map = tileset->_tile_map;

   std::vector<b2Vec2> vertices;
   std::vector<std::vector<uint32_t>> faces;

   for (auto y_tl = 0u; y_tl < height_tl; y_tl++)
   {
      const auto tile_offset_y_px = (offset_y_px + static_cast<int32_t>(y_tl)) * PIXELS_PER_TILE;

      for (auto x_tl = 0u; x_tl < width_tl; x_tl++)
      {
         const auto tile_offset_x_px = (offset_x_px + static_cast<int32_t>(x_tl)) * PIXELS_PER_TILE;
         const auto tile_number = tiles[y_tl * width_tl + x_tl];

         if (tile_number == 0)
         {
            continue;
         }

         auto tile_relative = -1;
         tile_relative = tile_number - tileset->_first_gid;
         auto tile_it = tile_map.find(tile_relative);

         if (tile_it != tile_map.end())
         {
            const auto& tile = tile_it->second;
            const auto& objects = tile->_object_group;

            if (!objects)
            {
               continue;
            }

            for (auto& object_it : objects->_objects)
            {
               auto object = object_it.second;
               auto poly = object->_polygon;
               auto line = object->_polyline;

               std::vector<sf::Vector2f> points;

               if (poly)
               {
                  points = poly->_polyline;
               }
               else if (line)
               {
                  points = line->_polyline;
               }
               else
               {
                  const auto x_px = object->_x_px;
                  const auto y_px = object->_y_px;
                  const auto w_px = object->_width_px;
                  const auto h_px = object->_height_px;

                  points = {
                     {x_px, y_px},
                     {x_px, y_px + h_px},
                     {x_px + w_px, y_px + h_px},
                     {x_px + w_px, y_px},
                  };
               }

               if (points.empty())
               {
                  continue;
               }

               std::vector<uint32_t> face;
               for (const auto& p : points)
               {
                  const auto px = tile_offset_x_px + p.x;
                  const auto py = tile_offset_y_px + p.y;
                  const auto v = b2Vec2(px, py);
                  const auto hash = computeHash(px, py);  // assume the TMX positions are actually all int or low precision
                  const auto face_it = face_indices.find(hash);
                  auto vertex_id = 0u;

                  if (face_it == face_indices.end())
                  {
                     vertex_id = static_cast<uint32_t>(vertices.size());
                     vertices.push_back(v);
                     face_indices[hash] = vertex_id;
                  }
                  else
                  {
                     vertex_id = face_it->second;
                  }

                  face.push_back(vertex_id + 1);  // wavefront obj starts indexing at 1
               }

               faces.push_back(face);
            }
         }
      }
   }

   if (vertices.empty())
   {
      Log::Error() << "tmx doesn't contain polygon data";
      return false;
   }

   // https://en.wikipedia.org/wiki/Wavefront_.obj_file
   Mesh::writeObj(path.string(), vertices, faces);

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

   return true;
}
