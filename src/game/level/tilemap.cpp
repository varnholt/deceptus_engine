#include "tilemap.h"

#include <ranges>

#include <math.h>
#include <algorithm>
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
#include "framework/tools/sfmlcompat.h"
#include "game/debug/drawcallcounter.h"
#include "game/io/texturepool.h"
#include "game/level/blendmodedeserializer.h"
#include "game/player/playerregistry.h"

namespace
{
constexpr auto tile_count_per_block = 16;
constexpr auto block_range_half_x = 3;
constexpr auto block_range_half_y = 2;

//! slack added around the view before the block window is derived from it, in pixels. the view used
//! for culling is the very one being rendered, so this only has to absorb rounding rather than any
//! camera movement, and is deliberately far smaller than the block size
constexpr auto block_margin_px = 48.0f;

std::array<int32_t, 2> getPlayerBlock()
{
   // draw the vertex arrays
   const auto& player_pos_px = PlayerRegistry::getFirst()->getPixelPositionInt();
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
   animated_tile->_x_tl = tx;
   animated_tile->_y_tl = ty;
   animated_tile->_animation = animation;

   auto duration = 0.0f;
   for (const auto& frame : frames)
   {
      auto offset_frame = new AnimatedTileFrame();
      offset_frame->_x_px = frame->_tile_id % (_texture_map->getSize().x / _tile_size_px.x);
      offset_frame->_y_px = frame->_tile_id / (_texture_map->getSize().x / _tile_size_px.x);
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

void TileMap::storeStaticVertices(const std::array<sf::Vertex, 4>& quad, float parallax_scale)
{
   // if no animation is available, just store the tile in the static buffer
   const auto bx = static_cast<int32_t>((quad[0].position.x / static_cast<float>(_tile_size_px.x) / parallax_scale) / tile_count_per_block);
   const auto by = static_cast<int32_t>((quad[0].position.y / static_cast<float>(_tile_size_px.y) / parallax_scale) / tile_count_per_block);

   auto y_it = _vertices_static_blocks.find(by);
   if (y_it == _vertices_static_blocks.end())
   {
      std::map<int32_t, sf::VertexArray> map;
      _vertices_static_blocks.insert(std::make_pair(by, map));
   }

   const auto x_it = _vertices_static_blocks[by].find(bx);
   if (x_it == _vertices_static_blocks[by].end())
   {
      _vertices_static_blocks[by][bx].setPrimitiveType(sf::PrimitiveType::Triangles);
   }

   sf::VertexArray& vertex_array = _vertices_static_blocks[by][bx];
   vertex_array.append(quad[0]);
   vertex_array.append(quad[1]);
   vertex_array.append(quad[2]);

   vertex_array.append(quad[0]);
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

      const auto post_lighting_it = map.find("post_lighting");
      if (post_lighting_it != map.end())
      {
         _post_lighting = post_lighting_it->second->_value_bool.value();
      }
   }

   // Log::Info() << "TileMap::load: loading tileset: " << tileSet->mName << " with: texture " << path;

   _tile_size_px = sf::Vector2u(tileset->_tile_width_px, tileset->_tile_height_px);
   _visible = layer->_visible;
   _z_index = layer->_z;

   _vertices_animated.setPrimitiveType(sf::PrimitiveType::Triangles);

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
         const auto tu = (tile_number - tileset->_first_gid) % (_texture_map->getSize().x / _tile_size_px.x);
         const auto tv = (tile_number - tileset->_first_gid) / (_texture_map->getSize().x / _tile_size_px.x);
         const auto tx = static_cast<int32_t>(pos_x);
         const auto ty = static_cast<int32_t>(pos_y);
         const auto tile_x_px = tx * static_cast<int32_t>(_tile_size_px.x) + layer->_position_x_px;
         const auto tile_y_px = ty * static_cast<int32_t>(_tile_size_px.y) + layer->_position_y_px;

         constexpr auto size = 1;

         // shrink UV range a TINY bit to avoid fetching data from undefined texture space
         const auto tile_eps_x = 0.5f * (1.0f / static_cast<float>(_tile_size_px.x));
         const auto tile_eps_y = 0.5f * (1.0f / static_cast<float>(_tile_size_px.y));

         // define its 4 corners
         // clang-format off
         std::array<sf::Vertex, 4> quad;
         quad[0].position = sf::Vector2f(static_cast<float>(tile_x_px), static_cast<float>(tile_y_px));
         quad[1].position = sf::Vector2f(static_cast<float>(tile_x_px + static_cast<int32_t>(_tile_size_px.x) * size), static_cast<float>(tile_y_px));
         quad[2].position = sf::Vector2f(static_cast<float>(tile_x_px + static_cast<int32_t>(_tile_size_px.x) * size), static_cast<float>(tile_y_px + static_cast<int32_t>(_tile_size_px.y) * size));
         quad[3].position = sf::Vector2f(static_cast<float>(tile_x_px), static_cast<float>(tile_y_px + static_cast<int32_t>(_tile_size_px.y) * size));
         
         quad[0].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size_px.x) + tile_eps_x, static_cast<float>(tv * _tile_size_px.y) + tile_eps_y);
         quad[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size_px.x) - tile_eps_x, static_cast<float>(tv * _tile_size_px.y) + tile_eps_y);
         quad[2].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size_px.x) - tile_eps_x, static_cast<float>((tv + 1) * _tile_size_px.y) - tile_eps_y);
         quad[3].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size_px.x) + tile_eps_x, static_cast<float>((tv + 1) * _tile_size_px.y) - tile_eps_y);

         const auto alpha = std::clamp(static_cast<int32_t>(layer->_opacity * 255.0f), 0, 255);
         quad[0].color = sf::Color(255, 255, 255, alpha);
         quad[1].color = sf::Color(255, 255, 255, alpha);
         quad[2].color = sf::Color(255, 255, 255, alpha);
         quad[3].color = sf::Color(255, 255, 255, alpha);
         // clang-format on

         auto it = tile_map.find(tile_number - tileset->_first_gid);
         if (it != tile_map.end() && it->second->_animation)
         {
            storeAnimation(quad, tx, ty, it->second->_animation);
         }
         else
         {
            storeStaticVertices(quad, parallax_scale);
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
      const auto bx = static_cast<int32_t>((anim->_x_tl) / tile_count_per_block);
      const auto by = static_cast<int32_t>((anim->_y_tl) / tile_count_per_block);
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

         index++;
      }

      auto frame = anim->_frames.at(index);

      const auto tu = static_cast<uint32_t>(frame->_x_px);
      const auto tv = static_cast<uint32_t>(frame->_y_px);

      // re-define its 4 texture coordinates
      anim->_vertices[0].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size_px.x), static_cast<float>(tv * _tile_size_px.y));
      anim->_vertices[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * _tile_size_px.x), static_cast<float>(tv * _tile_size_px.y));
      anim->_vertices[2].texCoords =
         sf::Vector2f(static_cast<float>((tu + 1) * _tile_size_px.x), static_cast<float>((tv + 1) * _tile_size_px.y));
      anim->_vertices[3].texCoords = sf::Vector2f(static_cast<float>(tu * _tile_size_px.x), static_cast<float>((tv + 1) * _tile_size_px.y));

      _vertices_animated.append(anim->_vertices[0]);
      _vertices_animated.append(anim->_vertices[1]);
      _vertices_animated.append(anim->_vertices[2]);

      _vertices_animated.append(anim->_vertices[0]);
      _vertices_animated.append(anim->_vertices[2]);
      _vertices_animated.append(anim->_vertices[3]);
   }
}

void TileMap::drawVertices(sf::RenderTarget& target, sf::RenderStates states, const std::vector<sf::FloatRect>& clip_rects_px) const
{
   states.transform *= getTransform();

#ifdef DEVELOPMENT_MODE
   if (DrawCallCounter::tilemap_last_target != &target)
   {
      DrawCallCounter::tilemap_target_switches++;
      DrawCallCounter::tilemap_last_target = &target;
   }
#endif

   // the block window follows the view that is actually being rendered rather than the player's
   // position. the two are not the same thing: the camera eases along behind the player, the
   // panorama pushes it further still, and a parallax layer is drawn through a view of its own that
   // sits at level_view * factor. culling around the player had to be padded until it covered all of
   // that, which is how a 640 x 360 view came to draw a 2304 x 1536 px window of tiles.
#ifdef DECEPTUS_VRSFML
   const auto& view = states.view;
#else
   const auto& view = target.getView();
#endif
   const auto view_center = sfcompat::getViewCenter(view);
   const auto view_size = sfcompat::getViewSize(view);

#ifdef DEVELOPMENT_MODE
   // the scissor is what a clipped pass actually pays for. the fill counter works in view space, so
   // without folding it in here a pass clipped to a light sprite reports the same overdraw as the
   // full screen pass it replaced - which is exactly what made the occluder clipping saving
   // invisible on the desktop instrument
   const auto clip_rect_px = DrawCallCounter::getClipRectPx(view);
   // and the target is what decides how many fragments a view pixel becomes: the same art drawn
   // into a half size normal target costs a quarter of the fill it costs in the colour target
   const auto target_fill_scale = DrawCallCounter::getTargetFillScale(target);
   const auto clip_left_px = clip_rect_px.position.x;
   const auto clip_top_px = clip_rect_px.position.y;
   const auto clip_right_px = clip_left_px + clip_rect_px.size.x;
   const auto clip_bottom_px = clip_top_px + clip_rect_px.size.y;
#endif

   // blocks are binned by the tileset's own tile size at load time, so the window has to be measured
   // in those units too - PIXELS_PER_TILE only happens to match for tilesets that use 24 px tiles
   // a tile map that never had a tileset assigned reports a zero tile size, and dividing by that
   // yields infinities that turn into meaningless block bounds
   if (_tile_size_px.x == 0 || _tile_size_px.y == 0)
   {
      return;
   }

   _batched_vertices.clear();

   const auto block_width_px = static_cast<float>(_tile_size_px.x * tile_count_per_block);
   const auto block_height_px = static_cast<float>(_tile_size_px.y * tile_count_per_block);

   const auto view_left_px = view_center.x - view_size.x * 0.5f - block_margin_px;
   const auto view_top_px = view_center.y - view_size.y * 0.5f - block_margin_px;
   const auto view_right_px = view_center.x + view_size.x * 0.5f + block_margin_px;
   const auto view_bottom_px = view_center.y + view_size.y * 0.5f + block_margin_px;

   const auto first_block_x = static_cast<int32_t>(std::floor(view_left_px / block_width_px));
   const auto last_block_x = static_cast<int32_t>(std::floor(view_right_px / block_width_px));
   const auto first_block_y = static_cast<int32_t>(std::floor(view_top_px / block_height_px));
   const auto last_block_y = static_cast<int32_t>(std::floor(view_bottom_px / block_height_px));

   // walking the blocks that exist inside the range, rather than probing every index in it, keeps
   // the work proportional to what is on screen instead of to the size of the range. that also makes
   // the loop immune to a nonsensical range: a degenerate view or tile size can produce bounds
   // spanning the whole int32 domain, and probing those index by index takes billions of lookups,
   // which is indistinguishable from a hang
   const auto row_end = _vertices_static_blocks.upper_bound(last_block_y);
   for (auto row_it = _vertices_static_blocks.lower_bound(first_block_y); row_it != row_end; ++row_it)
   {
      auto& row = row_it->second;
      const auto column_end = row.upper_bound(last_block_x);
      for (auto column_it = row.lower_bound(first_block_x); column_it != column_end; ++column_it)
      {
         const auto& block_vertices = column_it->second;
         const auto block_vertex_count = block_vertices.getVertexCount();

         // a block the caller cannot use is dropped before it is copied into the batch, so it costs
         // neither the copy nor the fill. the normal pass uses this: a block no light reaches
         // produces normals the deferred shader multiplies by a zero mask
         if (!clip_rects_px.empty())
         {
            const auto block_rect_px = sf::FloatRect{
               {static_cast<float>(column_it->first) * block_width_px, static_cast<float>(row_it->first) * block_height_px},
               {block_width_px, block_height_px}
            };

            const auto touches_clip_rect = std::ranges::any_of(
               clip_rects_px,
               [&block_rect_px](const auto& clip_rect_px)
               {
                  return block_rect_px.position.x < clip_rect_px.position.x + clip_rect_px.size.x &&
                         clip_rect_px.position.x < block_rect_px.position.x + block_rect_px.size.x &&
                         block_rect_px.position.y < clip_rect_px.position.y + clip_rect_px.size.y &&
                         clip_rect_px.position.y < block_rect_px.position.y + block_rect_px.size.y;
               }
            );

            if (!touches_clip_rect)
            {
               continue;
            }
         }

         if (block_vertex_count > 0)
         {
            _batched_vertices.insert(_batched_vertices.end(), &block_vertices[0], &block_vertices[0] + block_vertex_count);
         }
#ifdef DEVELOPMENT_MODE
         // blocks are drawn whole but only partly on screen, so scale the block's tile area by how
         // much of the block the view actually covers. counting the whole block would report fill
         // that the rasteriser never pays for
         const auto block_left_px = static_cast<float>(column_it->first) * block_width_px;
         const auto block_top_px = static_cast<float>(row_it->first) * block_height_px;
         const auto visible_width_px = std::max(
            0.0f,
            std::min(block_left_px + block_width_px, view_center.x + view_size.x * 0.5f) -
               std::max(block_left_px, view_center.x - view_size.x * 0.5f)
         );
         const auto visible_height_px = std::max(
            0.0f,
            std::min(block_top_px + block_height_px, view_center.y + view_size.y * 0.5f) -
               std::max(block_top_px, view_center.y - view_size.y * 0.5f)
         );
         const auto visible_fraction = (visible_width_px * visible_height_px) / (block_width_px * block_height_px);

         // the fraction the rasteriser is charged for, i.e. the block clipped to the scissor rather
         // than to the whole view. it stays separate from visible_fraction on purpose: that one is
         // the self check on the cull bounds and has to keep summing to view area over block area,
         // whatever a pass happens to be clipped to
         const auto rasterised_width_px =
            std::max(0.0f, std::min(block_left_px + block_width_px, clip_right_px) - std::max(block_left_px, clip_left_px));
         const auto rasterised_height_px =
            std::max(0.0f, std::min(block_top_px + block_height_px, clip_bottom_px) - std::max(block_top_px, clip_top_px));
         const auto rasterised_fraction = (rasterised_width_px * rasterised_height_px) / (block_width_px * block_height_px);

         // tiles are stored as two triangles, so six vertices make one tile - not four. dividing by
         // four overstated every count by 1.5x
         const auto tile_count = static_cast<float>(block_vertex_count / 6);
         DrawCallCounter::tilemap_pixels_submitted += static_cast<int64_t>(
            static_cast<double>(tile_count * rasterised_fraction * static_cast<float>(_tile_size_px.x * _tile_size_px.y)) *
            target_fill_scale
         );
         DrawCallCounter::tilemap_tiles_submitted += static_cast<int64_t>(tile_count * rasterised_fraction);
         DrawCallCounter::tilemap_blocks_drawn++;
         DrawCallCounter::tilemap_visible_fraction_sum += visible_fraction;
#endif
      }
   }

   // the animated tiles carry the same texture and blend mode as the static blocks, so they join
   // the same batch rather than paying for a call of their own
   const auto animated_vertex_count = _vertices_animated.getVertexCount();
   if (animated_vertex_count > 0)
   {
      _batched_vertices.insert(_batched_vertices.end(), &_vertices_animated[0], &_vertices_animated[0] + animated_vertex_count);
   }

   if (_batched_vertices.empty())
   {
      return;
   }

#ifdef DECEPTUS_VRSFML
   target.draw(std::span<const sf::Vertex>{_batched_vertices.data(), _batched_vertices.size()}, sf::PrimitiveType::Triangles, states);
#else
   target.draw(_batched_vertices.data(), _batched_vertices.size(), sf::PrimitiveType::Triangles, states);
#endif

#ifdef DEVELOPMENT_MODE
   DrawCallCounter::tilemap_draw_calls++;
   if (animated_vertex_count > 0)
   {
      DrawCallCounter::countAnimatedTilePixels(target, view, &_vertices_animated[0], animated_vertex_count);
   }
#endif
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

   if (_blend_mode.has_value())
   {
      states.blendMode = _blend_mode.value();
   }

   states.texture = _texture_map.get();
#ifdef DEVELOPMENT_MODE
   DrawCallCounter::beginTileMapLayer(_layer_name + "/" + _tileset_name);
#endif
   drawVertices(target, states);
#ifdef DEVELOPMENT_MODE
   DrawCallCounter::endTileMapLayer();
#endif
}

bool TileMap::dumpToPng(const std::filesystem::path& output_path) const
{
   if (!_texture_map || _vertices_static_blocks.empty())
   {
      std::cerr << "TileMap::dumpToPng - no texture or vertex data.\n";
      return false;
   }

   // determine bounds in pixels
   sf::FloatRect bounds;
   bool first = true;

   for (const auto& [by, row] : _vertices_static_blocks)
   {
      for (const auto& [bx, vertex_array] : row)
      {
         for (std::size_t i = 0; i < vertex_array.getVertexCount(); ++i)
         {
            const auto& pos = vertex_array[i].position;
            if (first)
            {
               bounds.position = pos;
               bounds.size = {pos.x, pos.y};
               first = false;
            }
            else
            {
               bounds.position.x = std::min(bounds.position.x, pos.x);
               bounds.position.y = std::min(bounds.position.y, pos.y);
               bounds.size.x = std::max(bounds.size.x, pos.x);
               bounds.size.y = std::max(bounds.size.y, pos.y);
            }
         }
      }
   }

   if (first)
   {
      std::cerr << "TileMap::dumpToPng - no vertices found.\n";
      return false;
   }

   // convert bottom-right max extents to actual size
   bounds.size.x -= bounds.position.x;
   bounds.size.y -= bounds.position.y;

   const sf::Vector2u render_size = {static_cast<uint32_t>(std::ceil(bounds.size.x)), static_cast<uint32_t>(std::ceil(bounds.size.y))};

   // create rendertexture
#ifndef DECEPTUS_VRSFML
   sf::RenderTexture render_texture(render_size);

   render_texture.clear(sf::Color::Transparent);

   sf::RenderStates states;
   states.texture = _texture_map.get();
   states.transform.translate(-bounds.position);  // align top-left to (0, 0)

   // draw static tiles
   for (const auto& [by, row] : _vertices_static_blocks)
   {
      for (const auto& [bx, vertex_array] : row)
      {
         render_texture.draw(vertex_array, states);
      }
   }

   // draw animated tiles
   if (_vertices_animated.getVertexCount() > 0)
   {
      render_texture.draw(_vertices_animated, states);
   }

   render_texture.display();

   // write to png
   const auto image = render_texture.getTexture().copyToImage();
   if (!image.saveToFile(output_path.string()))
   {
      std::cerr << "TileMap::dumpToPng - failed to save to file.\n";
      return false;
   }

   std::cout << "TileMap::dumpToPng - saved to " << output_path << "\n";
   return true;
#else
   return false;
#endif
}

void TileMap::draw(
   sf::RenderTarget& color,
   sf::RenderTarget& normal,
   sf::RenderStates states,
   const std::optional<sf::View>& normal_view,
   const std::vector<sf::FloatRect>& normal_clip_rects_px
) const
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

#ifdef DEVELOPMENT_MODE
   DrawCallCounter::beginTileMapLayer(_layer_name + "/" + _tileset_name);
#endif
   drawVertices(color, states);

   // the normal pass writes the same geometry a second time, and the deferred shader only ever
   // multiplies a normal by a light's sprite mask - so a normal the lights cannot reach is shaded
   // for nothing. an empty normal_view means no light reaches the screen at all, which makes the
   // whole second pass dead rather than merely oversized
   if (_normal_map && normal_view.has_value())
   {
      states.texture = _normal_map.get();

      auto normal_states = states;
#ifdef DECEPTUS_VRSFML
      normal_states.view = normal_view.value();
#else
      // the view lives on the target rather than in the states here, so it has to be put back:
      // the normal target is also blitted into outside this pass, and a leftover scissor would
      // clip that too
      const auto restore_view = normal.getView();
      normal.setView(normal_view.value());
#endif

#ifdef DEVELOPMENT_MODE
      DrawCallCounter::beginTileMapNormalPass();
#endif
      drawVertices(normal, normal_states, normal_clip_rects_px);
#ifdef DEVELOPMENT_MODE
      DrawCallCounter::endTileMapNormalPass();
#endif

#ifndef DECEPTUS_VRSFML
      normal.setView(restore_view);
#endif
   }

#ifdef DEVELOPMENT_MODE
   DrawCallCounter::endTileMapLayer();
#endif
}

bool TileMap::isPostLighting() const
{
   return _post_lighting;
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
   const auto& it =
      std::find_if(std::begin(_animations), std::end(_animations), [x, y](auto* tile) { return (tile->_x_tl == x && tile->_y_tl == y); });

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
