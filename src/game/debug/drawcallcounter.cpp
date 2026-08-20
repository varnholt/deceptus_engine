#include "drawcallcounter.h"

#ifdef DEVELOPMENT_MODE

#include <algorithm>
#include <ranges>

#include "framework/tools/sfmlcompat.h"

namespace
{

///
/// \brief Resolves the view a draw actually went through.
/// \param target render target the draw went to.
/// \param states render states the draw was made with.
/// \return the view in effect for that draw.
/// \note VRSFML carries the view in the render states rather than on the target, and a parallax
///       layer is drawn through a view of its own, so neither source alone is right everywhere.
///
const sf::View& resolveView(const sf::RenderTarget& target, const sf::RenderStates& states)
{
#ifdef DECEPTUS_VRSFML
   (void)target;
   return states.view;
#else
   (void)states;
   return target.getView();
#endif
}

///
/// \brief Area of an axis aligned rectangle clipped to the rasterised region, in pixels.
/// \param clip_px region of the view the draw is rasterised into.
/// \param left_px left edge of the rectangle.
/// \param top_px top edge of the rectangle.
/// \param right_px right edge of the rectangle.
/// \param bottom_px bottom edge of the rectangle.
/// \return the visible area, or zero when the rectangle is off screen.
///
int64_t visibleArea(const sf::FloatRect& clip_px, float left_px, float top_px, float right_px, float bottom_px)
{
   const auto clip_right_px = clip_px.position.x + clip_px.size.x;
   const auto clip_bottom_px = clip_px.position.y + clip_px.size.y;

   const auto visible_width_px = std::max(0.0f, std::min(right_px, clip_right_px) - std::max(left_px, clip_px.position.x));
   const auto visible_height_px = std::max(0.0f, std::min(bottom_px, clip_bottom_px) - std::max(top_px, clip_px.position.y));

   return static_cast<int64_t>(visible_width_px * visible_height_px);
}

///
/// \brief Sums the on-screen area of a run of axis aligned quads.
/// \param clip_px region of the view the quads are rasterised into.
/// \param vertices first vertex of the run.
/// \param vertex_count how many vertices the run holds.
/// \return the visible area of every whole quad in the run.
/// \note two triangles per quad, so six vertices: top left, top right, bottom right, then top
///       left, bottom right, bottom left. Both the ao atlas and the animated tiles build them
///       that way.
///
int64_t sumVisibleQuadArea(const sf::FloatRect& clip_px, const sf::Vertex* vertices, std::size_t vertex_count)
{
   int64_t visible_px = 0;
   for (std::size_t vertex_index = 0; vertex_index + 6 <= vertex_count; vertex_index += 6)
   {
      const auto& top_left = vertices[vertex_index].position;
      const auto& bottom_right = vertices[vertex_index + 2].position;
      visible_px += visibleArea(clip_px, top_left.x, top_left.y, bottom_right.x, bottom_right.y);
   }
   return visible_px;
}

//! tilemap_pixels_submitted as it stood when the normal pass started, so the pass can be measured
//! without threading a flag through TileMap::drawVertices
int64_t tilemap_pixels_before_normal_pass = 0;

//! the same trick one level up, for the layer currently being drawn. held by value: the caller
//! may well pass a temporary, and a pointer to one dangles the moment the call returns
int64_t tilemap_pixels_before_layer = 0;
int64_t tilemap_tiles_before_layer = 0;
int32_t tilemap_blocks_before_layer = 0;
double tilemap_fraction_before_layer = 0.0;
std::string current_layer_name;
bool layer_attribution_active = false;

}  // namespace

void DrawCallCounter::beginTileMapLayer(const std::string& layer_name)
{
   current_layer_name = layer_name;
   layer_attribution_active = true;
   tilemap_pixels_before_layer = tilemap_pixels_submitted;
   tilemap_tiles_before_layer = tilemap_tiles_submitted;
   tilemap_blocks_before_layer = tilemap_blocks_drawn;
   tilemap_fraction_before_layer = tilemap_visible_fraction_sum;
}

void DrawCallCounter::endTileMapLayer()
{
   if (!layer_attribution_active)
   {
      return;
   }

   const auto layer_name = current_layer_name;
   layer_attribution_active = false;

   const auto submitted = tilemap_pixels_submitted - tilemap_pixels_before_layer;
   const auto tiles = tilemap_tiles_submitted - tilemap_tiles_before_layer;
   const auto blocks = tilemap_blocks_drawn - tilemap_blocks_before_layer;
   const auto fraction = tilemap_visible_fraction_sum - tilemap_fraction_before_layer;
   if (submitted == 0)
   {
      return;
   }

   // a level has a few dozen layers, so a linear scan beats a map, and appending in first-drawn
   // order keeps two reports comparable line by line
   const auto layer_it =
      std::ranges::find_if(tilemap_layer_pixels, [&layer_name](const auto& entry) { return entry._layer_name == layer_name; });

   if (layer_it == tilemap_layer_pixels.end())
   {
      tilemap_layer_pixels.push_back({layer_name, submitted, 1, tiles, blocks, fraction});
      return;
   }

   layer_it->_pixels_submitted += submitted;
   layer_it->_draw_count++;
   layer_it->_tiles_submitted += tiles;
   layer_it->_blocks_drawn += blocks;
   layer_it->_visible_fraction_sum += fraction;
}

void DrawCallCounter::beginTileMapNormalPass()
{
   tilemap_pixels_before_normal_pass = tilemap_pixels_submitted;
}

void DrawCallCounter::endTileMapNormalPass()
{
   tilemap_normal_pixels_submitted += tilemap_pixels_submitted - tilemap_pixels_before_normal_pass;
}

sf::FloatRect DrawCallCounter::getClipRectPx(const sf::View& view)
{
   const auto view_center = sfcompat::getViewCenter(view);
   const auto view_size = sfcompat::getViewSize(view);
   const auto scissor = sfcompat::getViewScissor(view);

   return sf::FloatRect{
      {view_center.x + (scissor.position.x - 0.5f) * view_size.x, view_center.y + (scissor.position.y - 0.5f) * view_size.y},
      {scissor.size.x * view_size.x, scissor.size.y * view_size.y}
   };
}

void DrawCallCounter::countAnimatedTilePixels(const sf::View& view, const sf::Vertex* vertices, std::size_t vertex_count)
{
   tilemap_pixels_submitted += sumVisibleQuadArea(getClipRectPx(view), vertices, vertex_count);
}

void DrawCallCounter::countAmbientOcclusionPixels(
   const sf::RenderTarget& target,
   const sf::RenderStates& states,
   const std::vector<sf::Vertex>& batched_vertices
)
{
   const auto& view = resolveView(target, states);

   // the chunks are gathered around the player rather than around the view, so a good part of the
   // batch never reaches the screen. clipping each quad is what keeps this comparable to the tile
   // count, which is measured the same way
   ambient_occlusion_pixels_submitted += sumVisibleQuadArea(getClipRectPx(view), batched_vertices.data(), batched_vertices.size());
}

void DrawCallCounter::countImageLayerPixels(const sf::RenderTarget& target, const sf::RenderStates& states, const sf::Sprite& sprite)
{
   const auto& view = resolveView(target, states);
   const auto bounds = sprite.getGlobalBounds();

   image_layer_pixels_submitted += visibleArea(
      getClipRectPx(view), bounds.position.x, bounds.position.y, bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y
   );
}

#endif  // DEVELOPMENT_MODE
