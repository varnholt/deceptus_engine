#include "drawcallcounter.h"

#ifdef DEVELOPMENT_MODE

#include <algorithm>

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
/// \brief Area of an axis aligned rectangle clipped to the view, in pixels.
/// \param view_center centre of the view being rendered through.
/// \param view_size size of the view being rendered through.
/// \param left_px left edge of the rectangle.
/// \param top_px top edge of the rectangle.
/// \param right_px right edge of the rectangle.
/// \param bottom_px bottom edge of the rectangle.
/// \return the visible area, or zero when the rectangle is off screen.
///
int64_t
visibleArea(const sf::Vector2f& view_center, const sf::Vector2f& view_size, float left_px, float top_px, float right_px, float bottom_px)
{
   const auto view_left_px = view_center.x - view_size.x * 0.5f;
   const auto view_top_px = view_center.y - view_size.y * 0.5f;
   const auto view_right_px = view_center.x + view_size.x * 0.5f;
   const auto view_bottom_px = view_center.y + view_size.y * 0.5f;

   const auto visible_width_px = std::max(0.0f, std::min(right_px, view_right_px) - std::max(left_px, view_left_px));
   const auto visible_height_px = std::max(0.0f, std::min(bottom_px, view_bottom_px) - std::max(top_px, view_top_px));

   return static_cast<int64_t>(visible_width_px * visible_height_px);
}

//! tilemap_pixels_submitted as it stood when the normal pass started, so the pass can be measured
//! without threading a flag through TileMap::drawVertices
int64_t tilemap_pixels_before_normal_pass = 0;

}  // namespace

void DrawCallCounter::beginTileMapNormalPass()
{
   tilemap_pixels_before_normal_pass = tilemap_pixels_submitted;
}

void DrawCallCounter::endTileMapNormalPass()
{
   tilemap_normal_pixels_submitted += tilemap_pixels_submitted - tilemap_pixels_before_normal_pass;
}

void DrawCallCounter::countAmbientOcclusionPixels(
   const sf::RenderTarget& target,
   const sf::RenderStates& states,
   const std::vector<sf::Vertex>& batched_vertices
)
{
   const auto& view = resolveView(target, states);
   const auto view_center = sfcompat::getViewCenter(view);
   const auto view_size = sfcompat::getViewSize(view);

   // the chunks are gathered around the player rather than around the view, so a good part of the
   // batch never reaches the screen. clipping each quad is what keeps this comparable to the tile
   // count, which is measured the same way.
   //
   // two triangles per quad, so six vertices: top left, top right, bottom right, then top left,
   // bottom right, bottom left - see how AmbientOcclusion::load builds them
   for (auto vertex_index = 0u; vertex_index + 6 <= batched_vertices.size(); vertex_index += 6)
   {
      const auto& top_left = batched_vertices[vertex_index].position;
      const auto& bottom_right = batched_vertices[vertex_index + 2].position;

      ambient_occlusion_pixels_submitted += visibleArea(view_center, view_size, top_left.x, top_left.y, bottom_right.x, bottom_right.y);
   }
}

void DrawCallCounter::countImageLayerPixels(const sf::RenderTarget& target, const sf::RenderStates& states, const sf::Sprite& sprite)
{
   const auto& view = resolveView(target, states);
   const auto view_center = sfcompat::getViewCenter(view);
   const auto view_size = sfcompat::getViewSize(view);

   const auto bounds = sprite.getGlobalBounds();

   image_layer_pixels_submitted += visibleArea(
      view_center, view_size, bounds.position.x, bounds.position.y, bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y
   );
}

#endif  // DEVELOPMENT_MODE
