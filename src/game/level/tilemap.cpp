#include "tilemap.h"

#include <math.h>
#include <iostream>
#include <map>

// tmx
#include "framework/tmxparser/tmxanimation.h"
#include "framework/tmxparser/tmxframe.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtile.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/level/blendmodedeserializer.h"
#include "game/player/player.h"

namespace
{
constexpr auto tile_count_per_block = 16;
constexpr auto block_range_half_x = 3;
constexpr auto block_range_half_y = 2;

std::array<int32_t, 2> getPlayerBlock()
{
   // draw the vertex arrays
   const auto& player_pos_px = Player::getCurrent()->getPixelPositionInt();
   return {(player_pos_px.x / PIXELS_PER_TILE) / tile_count_per_block, (player_pos_px.y / PIXELS_PER_TILE) / tile_count_per_block};
}

}  // namespace

TileMap::~TileMap()
{
   _vertices_animated.clear();

   for (auto& [k1, v1] : _vertices_static_blocks)
   {
      for (auto& [k2, v2] : v1)
      {
         v2.clear();
      }
   }
}

bool TileMap::isVisible() const
{
   return _visible;
}

void TileMap::setVisible(bool visible)
{
   _visible = visible;
}

void TileMap::storeAnimation(const std::array<sf::Vertex, 4>& quad, int32_t tx, int32_t ty, const std::shared_ptr<TmxAnimation>& animation)
{
   const auto& frames = animation->_frames;

   auto animated_tile = new AnimatedTile();
   animated_tile->_tile_x = tx;
   animated_tile->_tile_y = ty;
   animated_tile->_animation = animation;

   auto duration = 0.0f;
   for (const auto& frame : frames)
   {
      auto offset_frame = new AnimatedTileFrame();
      offset_frame->_x_px = frame->_tile_id % (_texture_map->getSize().x / _tile_size.x);
      offset_frame->_y_px = frame->_tile_id / (_texture_map->getSize().x / _tile_size.x);
      offset_frame->_duration_ms = frame->_duration_ms;
      animated_tile->_frames.push_back(offset_frame);
      duration += frame->_duration_ms;
   }

   animated_tile->_duration = duration;

   animated_tile->_vertices[0] = quad[0];
   animated_tile->_vertices[1] = quad[1];
   animated_tile->_vertices[2] = quad[2];
   animated_tile->_vertices[3] = quad[3];

   _animations.push_back(animated_tile);
}

void TileMap::storeStaticVertices(const std::array<sf::Vertex, 4>& quad, const int32_t tx, const int32_t ty, float parallax_scale)
{
   // if no animation is available, just store the tile in the static buffer
   const auto bx = static_cast<int32_t>((tx / parallax_scale) / tile_count_per_block);
   const auto by = static_cast<int32_t>((ty / parallax_scale) / tile_count_per_block);

   auto y_it = _vertices_static_blocks.find(by);
   if (y_it == _vertices_static_blocks.end())
   {
      std::map<int32_t, sf::VertexArray> map;
      _vertices_static_blocks.insert(std::make_pair(by, map));
   }

   const auto x_it = _vertices_static_blocks[by].find(bx);
   if (x_it == _vertices_static_blocks[by].end())
   {
      _vertices_static_blocks[by][bx].setPrimitiveType(sf::Quads);
   }

   sf::VertexArray& vertex_array = _vertices_static_blocks[by][bx];
   vertex_array.append(quad[0]);
   vertex_array.append(quad[1]);
   vertex_array.append(quad[2]);
   vertex_array.append(quad[3]);
}

bool TileMap::load(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileset,
   const std::filesystem::path& base_path
)
{
   if (!tileset)
   {
      return false;
   }

   _layer_name = layer->_name;
   _tileset_name = tileset->_name;

   auto path = (base_path / tileset->_image->_source);

   _texture_map = TexturePool::getInstance().get(path);

   // check if we have a bumpmap and, if so, load it
   const auto normal_map_filename = (path.stem().string() + "_normals" + path.extension().string());
   const auto normal_map_path = (path.parent_path() / normal_map_filename);
   if (std::filesystem::exists(normal_map_path))
   {
      // Log::Info() << "found normal map for " << path.string();
      _normal_map = TexturePool::getInstance().get(normal_map_path);
   }

   auto parallax_scale = 1.0f;
   if (layer->_properties)
   {
      const auto& map = layer->_properties->_map;

      const auto it_parallax_value = map.find("parallax");
      if (it_parallax_value != map.end())
      {
         parallax_scale = it_parallax_value->second->_value_float.value();
      }

      _blend_mode = BlendModeDeserializer::readBlendMode(map);
   }

   // Log::Info() << "TileMap::load: loading tileset: " << tileSet->mName << " with: texture " << path;

   _tile_size = sf::Vector2u(tileset->_tile_width_px, tileset->_tile_height_px);
   _visible = layer->_visible;
   _z_index = layer->_z;

   _vertices_animated.setPrimitiveType(sf::Quads);

   auto& tile_map = tileset->_tile_map;

   // populate the vertex array, with one quad per tile
   for (auto pos_x = 0u; pos_x < layer->_width_tl; ++pos_x)
   {
      for (auto pos_y = 0u; pos_y < layer->_height_tl; ++pos_y)
      {
         // get the current tile number
         const auto tile_number = layer->_data[pos_x + pos_y * layer->_width_tl];

         if (tile_number == 0)
         {
            continue;
         }

         // find its position in the tileset texture
         const auto tu = (tile_number - tileset->_first_gid) % (_texture_map->getSize().x / _tile_size.x);
         const auto tv = (tile_number - tileset->_first_gid) / (_texture_map->getSize().x / _tile_size.x);
         const auto tx = pos_x + layer->_offset_x_px;
         const auto ty = pos_y + layer->_offset_y_px;

         constexpr auto size = 1;

         // shrink UV range a TINY bit to avoid fetching data from undefined texture space
         const auto tile_eps_x = 0.5f * (1.0f / static_cast<float>(_tile_size.x));
         const auto tile_eps_y = 0.5f * (1.0f / static_cast<float>(_tile_size.y));

         // define its 4 corners
         // clang-format off
         std::array<sf::Vertex, 4> quad;
         quad[0].position = sf::Vector2f(static_cast<float>(tx * _tile_size.x), static_cast<float>(ty * _tile_size.y));
         quad[1].position = sf::Vector2f(static_cast<float>((tx + size) * _tile_size.x), static_cast<float>(ty * _tile_size.y));
         quad[2].position = sf::Vector2f(static_cast<float>((tx + size) * _tile_size.x), static_cast<float>((ty + size) * _tile_size.y));
         quad[3].position = sf::Vector2f(static_cast<float>(tx * _tile_size.x), static_cast<float>((ty + size) * _tile_size.y));
         quad[0].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size.x) + tile_eps_x, static_cast<float>(tv * _tile_size.y) + tile_eps_y);
         quad[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size.x) - tile_eps_x, static_cast<float>(tv * _tile_size.y) + tile_eps_y);
         quad[2].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size.x) - tile_eps_x, static_cast<float>((tv + 1) * _tile_size.y) - tile_eps_y);
         quad[3].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size.x) + tile_eps_x, static_cast<float>((tv + 1) * _tile_size.y) - tile_eps_y);
         quad[0].color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer->_opacity * 255.0f));
         quad[1].color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer->_opacity * 255.0f));
         quad[2].color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer->_opacity * 255.0f));
         quad[3].color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer->_opacity * 255.0f));
         // clang-format on

         auto it = tile_map.find(tile_number - tileset->_first_gid);
         if (it != tile_map.end() && it->second->_animation)
         {
            storeAnimation(quad, tx, ty, it->second->_animation);
         }
         else
         {
            storeStaticVertices(quad, tx, ty, parallax_scale);
         }
      }
   }

   return true;
}

void TileMap::update(const sf::Time& dt)
{
   _vertices_animated.clear();

   const auto player_block = getPlayerBlock();

   for (auto& anim : _animations)
   {
      if (!anim->_visible)
      {
         continue;
      }

      // only add those that are close enough to the player
      const auto bx = static_cast<int32_t>((anim->_tile_x) / tile_count_per_block);
      const auto by = static_cast<int32_t>((anim->_tile_y) / tile_count_per_block);
      if (std::abs(player_block[0] - bx) > block_range_half_x || std::abs(player_block[1] - by) > block_range_half_y)
      {
         continue;
      }

      anim->_elapsed_ms += dt.asMilliseconds();
      anim->_elapsed_ms = fmod(anim->_elapsed_ms, anim->_duration);

      auto index = 0u;
      float frame_duration = 0.0f;
      for (auto* frame : anim->_frames)
      {
         frame_duration += frame->_duration_ms;

         if (frame_duration > anim->_elapsed_ms)
         {
            break;
         }
         else
         {
            index++;
         }
      }

      auto frame = anim->_frames.at(index);

      const auto tu = static_cast<uint32_t>(frame->_x_px);
      const auto tv = static_cast<uint32_t>(frame->_y_px);

      // re-define its 4 texture coordinates
      anim->_vertices[0].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size.x), static_cast<float>(tv * _tile_size.y));
      anim->_vertices[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size.x), static_cast<float>(tv * _tile_size.y));
      anim->_vertices[2].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size.x), static_cast<float>((tv + 1) * _tile_size.y));
      anim->_vertices[3].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size.x), static_cast<float>((tv + 1) * _tile_size.y));

      _vertices_animated.append(anim->_vertices[0]);
      _vertices_animated.append(anim->_vertices[1]);
      _vertices_animated.append(anim->_vertices[2]);
      _vertices_animated.append(anim->_vertices[3]);
   }
}

void TileMap::drawVertices(sf::RenderTarget& target, sf::RenderStates states) const
{
   states.transform *= getTransform();

   const auto player_block = getPlayerBlock();
   const auto bx = player_block[0];
   const auto by = player_block[1];

   for (auto iy = by - block_range_half_y; iy < by + block_range_half_y; iy++)
   {
      const auto y_it = _vertices_static_blocks.find(iy);
      if (y_it != _vertices_static_blocks.end())
      {
         for (auto ix = bx - block_range_half_x; ix < bx + block_range_half_x; ix++)
         {
            const auto x_it = _vertices_static_blocks[iy].find(ix);
            if (x_it != _vertices_static_blocks[iy].end())
            {
               target.draw(x_it->second, states);
            }
         }
      }
   }

   target.draw(_vertices_animated, states);
}

const std::string& TileMap::getLayerName() const
{
   return _layer_name;
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   states.texture = _texture_map.get();
   drawVertices(target, states);
}

void TileMap::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   states.texture = _texture_map.get();

   if (_blend_mode.has_value())
   {
      states.blendMode = _blend_mode.value();
   }

   drawVertices(color, states);

   if (_normal_map)
   {
      states.texture = _normal_map.get();
      drawVertices(normal, states);
   }
}

int TileMap::getZ() const
{
   return _z_index;
}

void TileMap::setZ(int32_t z)
{
   _z_index = z;
}

void TileMap::hideTile(int32_t x, int32_t y)
{
   const auto& it = std::find_if(
      std::begin(_animations), std::end(_animations), [x, y](auto* tile) { return (tile->_tile_x == x && tile->_tile_y == y); }
   );

   if (it != _animations.end())
   {
      (*it)->_visible = false;
   }
   else
   {
      const auto bx = static_cast<int32_t>(x / tile_count_per_block);
      const auto by = static_cast<int32_t>(y / tile_count_per_block);

      const auto& y_it = _vertices_static_blocks.find(by);
      if (y_it != _vertices_static_blocks.end())
      {
         const auto& x_it = _vertices_static_blocks[by].find(bx);
         if (x_it != _vertices_static_blocks[by].end())
         {
            auto& vertices = x_it->second;
            for (auto i = 0u; i < vertices.getVertexCount(); i += 4)
            {
               if (static_cast<int32_t>(vertices[i].position.x) / PIXELS_PER_TILE == x &&
                   static_cast<int32_t>(vertices[i].position.y) / PIXELS_PER_TILE == y)
               {
                  vertices[i].color.a = 0;
                  vertices[i + 1].color.a = 0;
                  vertices[i + 2].color.a = 0;
                  vertices[i + 3].color.a = 0;
               }
            }
         }
      }
   }
}

TileMap::AnimatedTile::~AnimatedTile()
{
   _frames.clear();
}
