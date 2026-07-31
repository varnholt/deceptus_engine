#include "levelmap.h"

#include "framework/tools/log.h"
#include "game/io/meshtools.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{

//! map pixels per tile at the most detailed level
constexpr auto base_map_px_per_tile = 3;

//! how many base cells collapse into one map pixel, one entry per detail level.
//! four entries, matching the zoom_level_* indicator layers of the map page.
constexpr std::array<int32_t, 4> detail_level_block_sizes = {1, 2, 3, 4};

/// \brief one mesh edge prepared for scanline traversal.
struct ScanlineEdge
{
   float _y_min = 0.0f;
   float _y_max = 0.0f;
   float _x_at_y_min = 0.0f;
   float _slope = 0.0f;
};

}  // namespace

bool LevelMap::build(const std::filesystem::path& obj_path, int32_t width_tl, int32_t height_tl, int32_t tile_size_px)
{
   if (width_tl <= 0 || height_tl <= 0 || tile_size_px <= 0)
   {
      Log::Error() << "invalid level dimensions for map generation";
      return false;
   }

   _base_width = width_tl * base_map_px_per_tile;
   _base_height = height_tl * base_map_px_per_tile;
   _base_world_px_per_map_px = static_cast<float>(tile_size_px) / static_cast<float>(base_map_px_per_tile);

   if (!rasterize(obj_path))
   {
      return false;
   }

   const auto maximum_size = static_cast<int32_t>(sf::Texture::getMaximumSize());

   for (const auto block_size : detail_level_block_sizes)
   {
      // very large levels would exceed the maximum texture size, those detail levels are skipped
      if (_base_width / block_size > maximum_size || _base_height / block_size > maximum_size)
      {
         Log::Warning() << "skipping map detail level with block size " << block_size << ", texture would be too large";
         continue;
      }

      addDetailLevel(block_size);
   }

   return !_detail_levels.empty();
}

bool LevelMap::rasterize(const std::filesystem::path& obj_path)
{
   std::vector<b2Vec2> points;
   std::vector<std::vector<uint32_t>> faces;
   Mesh::readObj(obj_path.string(), points, faces);

   if (points.empty() || faces.empty())
   {
      Log::Error() << "no mesh data in " << obj_path.string() << ", map will not be generated";
      return false;
   }

   std::vector<ScanlineEdge> edges;
   for (const auto& face : faces)
   {
      const auto point_count = face.size();
      for (auto index = 0u; index < point_count; index++)
      {
         // readObj already converts the wavefront 1-based indices to 0-based ones
         const auto& start = points[face[index]];
         const auto& end = points[face[(index + 1) % point_count]];

         // horizontal edges never cross a scanline
         if (start.y == end.y)
         {
            continue;
         }

         const auto& upper = (start.y < end.y) ? start : end;
         const auto& lower = (start.y < end.y) ? end : start;

         edges.push_back(ScanlineEdge{upper.y, lower.y, upper.x, (lower.x - upper.x) / (lower.y - upper.y)});
      }
   }

   if (edges.empty())
   {
      Log::Error() << "mesh in " << obj_path.string() << " has no usable edges";
      return false;
   }

   // everything the mesh does not enclose is walkable
   _interior.assign(static_cast<size_t>(_base_width) * static_cast<size_t>(_base_height), true);

   std::vector<float> crossings;
   crossings.reserve(64);

   for (auto row = 0; row < _base_height; row++)
   {
      const auto scanline_y_px = (static_cast<float>(row) + 0.5f) * _base_world_px_per_map_px;

      crossings.clear();
      for (const auto& edge : edges)
      {
         if (edge._y_min <= scanline_y_px && edge._y_max > scanline_y_px)
         {
            crossings.push_back(edge._x_at_y_min + (scanline_y_px - edge._y_min) * edge._slope);
         }
      }

      if (crossings.empty())
      {
         continue;
      }

      std::sort(crossings.begin(), crossings.end());

      // even-odd rule: everything between crossing 0 and 1, 2 and 3, ... is inside the mesh
      const auto row_offset = static_cast<size_t>(row) * static_cast<size_t>(_base_width);
      for (auto crossing_index = 0u; crossing_index + 1 < crossings.size(); crossing_index += 2)
      {
         const auto span_start = static_cast<int32_t>(std::ceil(crossings[crossing_index] / _base_world_px_per_map_px - 0.5f));
         const auto span_end = static_cast<int32_t>(std::ceil(crossings[crossing_index + 1] / _base_world_px_per_map_px - 0.5f));

         const auto clamped_start = std::max(0, span_start);
         const auto clamped_end = std::min(_base_width, span_end);

         for (auto column = clamped_start; column < clamped_end; column++)
         {
            _interior[row_offset + static_cast<size_t>(column)] = false;
         }
      }
   }

   return true;
}

void LevelMap::addDetailLevel(int32_t block_size)
{
   const auto width = _base_width / block_size;
   const auto height = _base_height / block_size;

   if (width <= 0 || height <= 0)
   {
      return;
   }

   // a merged cell counts as walkable when any of its base cells is. that way a one tile wide
   // corridor stays on the map no matter how far the view is zoomed out, and walls between two
   // chambers collapse into the single pixel line that separates them.
   std::vector<bool> interior(static_cast<size_t>(width) * static_cast<size_t>(height), false);

   for (auto y = 0; y < height; y++)
   {
      for (auto x = 0; x < width; x++)
      {
         auto walkable = false;

         for (auto block_y = 0; block_y < block_size && !walkable; block_y++)
         {
            const auto base_y = y * block_size + block_y;
            for (auto block_x = 0; block_x < block_size; block_x++)
            {
               const auto base_x = x * block_size + block_x;
               if (_interior[static_cast<size_t>(base_y) * static_cast<size_t>(_base_width) + static_cast<size_t>(base_x)])
               {
                  walkable = true;
                  break;
               }
            }
         }

         interior[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = walkable;
      }
   }

   const auto is_interior = [&interior, width, height](int32_t x, int32_t y) -> bool
   {
      if (x < 0 || y < 0 || x >= width || y >= height)
      {
         return false;
      }
      return interior[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
   };

   // the wall skin is derived per detail level, so it is always exactly one map pixel wide.
   // solid cells further inside stay background, which is what makes the map read as an outline.
   std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

   for (auto y = 0; y < height; y++)
   {
      for (auto x = 0; x < width; x++)
      {
         const auto index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);

         auto color = _style._background;

         if (is_interior(x, y))
         {
            color = _style._interior;
         }
         else if (is_interior(x - 1, y) || is_interior(x + 1, y) || is_interior(x, y - 1) || is_interior(x, y + 1))
         {
            color = _style._wall;
         }

         pixels[index * 4 + 0] = color.r;
         pixels[index * 4 + 1] = color.g;
         pixels[index * 4 + 2] = color.b;
         pixels[index * 4 + 3] = color.a;
      }
   }

   const auto texture_size = sf::Vector2u{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

#ifdef __EMSCRIPTEN__
   auto created_texture = sf::Texture::create(texture_size);
   if (!created_texture.hasValue())
   {
      Log::Error() << "failed to create level map texture";
      return;
   }
   auto texture = std::make_shared<sf::Texture>(std::move(*created_texture));
#else
   auto texture = std::make_shared<sf::Texture>(texture_size);
#endif

   texture->update(pixels.data());

   _detail_levels.push_back(DetailLevel{texture, _base_world_px_per_map_px * static_cast<float>(block_size)});

   Log::Info() << "generated level map detail level " << _detail_levels.size() - 1 << ": " << width << "x" << height << " px ("
               << static_cast<float>(base_map_px_per_tile) / static_cast<float>(block_size) << " px per tile)";
}

bool LevelMap::isValid() const
{
   return !_detail_levels.empty();
}

size_t LevelMap::getDetailLevelCount() const
{
   return _detail_levels.size();
}

const sf::Texture* LevelMap::getTexture(size_t detail_level) const
{
   if (detail_level >= _detail_levels.size())
   {
      return nullptr;
   }

   return _detail_levels[detail_level]._texture.get();
}

sf::Vector2u LevelMap::getSize(size_t detail_level) const
{
   if (detail_level >= _detail_levels.size())
   {
      return {};
   }

   return _detail_levels[detail_level]._texture->getSize();
}

float LevelMap::getWorldPixelsPerMapPixel(size_t detail_level) const
{
   if (detail_level >= _detail_levels.size())
   {
      return 0.0f;
   }

   return _detail_levels[detail_level]._world_px_per_map_px;
}

sf::Vector2f LevelMap::toMap(const sf::Vector2f& position_px, size_t detail_level) const
{
   if (detail_level >= _detail_levels.size())
   {
      return {};
   }

   return position_px / _detail_levels[detail_level]._world_px_per_map_px;
}

sf::FloatRect LevelMap::toMap(const sf::FloatRect& rect_px, size_t detail_level) const
{
   if (detail_level >= _detail_levels.size())
   {
      return {};
   }

   const auto scale = _detail_levels[detail_level]._world_px_per_map_px;
   return sf::FloatRect{rect_px.position / scale, rect_px.size / scale};
}
