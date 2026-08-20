#pragma once

#ifdef DEVELOPMENT_MODE

#include "SFML/Graphics.hpp"

#include <cstdint>
#include <string>
#include <vector>

///
/// \brief Counts the draw calls tile maps issue, so the profiler can report them per frame.
///
/// Tile map geometry is submitted one vertex array per 16x16 tile block, into the color target and
/// again into the normal target, for every tile map and every z index. That makes the call count
/// the thing to watch when a frame is bound by submission rather than by fill.
///
namespace DrawCallCounter
{
inline int32_t tilemap_draw_calls = 0;  //!< reset once per frame by the profiler

//! Times the render target changed between two consecutive tile map submissions. Tile maps that
//! carry a normal map are drawn into the colour target and then into the normal one, so the target
//! alternates inside the z loop; each change costs a framebuffer bind.
inline int32_t tilemap_target_switches = 0;
inline const void* tilemap_last_target = nullptr;

//! Draw calls the ambient occlusion layer issues. Kept apart from the tile map count because they
//! come from a different system: ao is a chunked set of quads sharing one atlas, and it used to
//! submit one call per quad - a median of 230 per frame in the catacombs against 50 for every tile
//! map put together. That went unnoticed for as long as the counter only described tile maps.
inline int32_t ambient_occlusion_draw_calls = 0;

//! Candidates examined by Level::drawLayers while looking for things to draw at a z index. The loop
//! runs once per z index and rescans every container each time, so this grows with the z range
//! multiplied by the level's content rather than with what is actually on screen.
inline int32_t layer_scan_steps = 0;

//! Tile pixels submitted per frame, counted as drawn tiles multiplied by the tile area. Held against
//! the view area it gives the overdraw factor: how many times the average pixel on screen is written
//! by tile geometry alone. This is a pure count, so it reads the same on hardware as in an emulator,
//! which makes it the one way to size a fill problem without a console.
inline int64_t tilemap_pixels_submitted = 0;

//! tiles submitted this frame, the same count without the per tile area factor
inline int64_t tilemap_tiles_submitted = 0;

//! blocks handed to the rasteriser this frame, and the sum of how much of each the view covers.
//! the fractions have to add up to view area over block area - about 1.56 for a 640x360 view and
//! 384 px blocks. anything above that means block rectangles are overlapping, i.e. the bounds the
//! cull derives do not match where the block content actually is
inline int32_t tilemap_blocks_drawn = 0;
inline double tilemap_visible_fraction_sum = 0.0;

//! Ambient occlusion pixels submitted per frame, each quad clipped to the view. Kept apart from the
//! tile count because ao is a full screen overlay of thousands of alpha blended quads, so it costs
//! fill out of proportion to the single draw call it now takes.
inline int64_t ambient_occlusion_pixels_submitted = 0;

//! Image layer pixels submitted per frame. The parallax backdrops are drawn to fill the view, so
//! each one that is visible writes roughly a whole screen; the catacombs carry 21 of them.
inline int64_t image_layer_pixels_submitted = 0;

//! The part of tilemap_pixels_submitted that went to the normal target. Tile maps carrying a normal
//! map are drawn a second time, so these pixels are shaded twice for one visible result - on a fill
//! bound machine that is the difference between the two passes, whatever the draw call count says.
inline int64_t tilemap_normal_pixels_submitted = 0;

///
/// \brief One tile map layer and the pixels it has submitted since the last report.
///
struct TileMapLayerPixels
{
   std::string _layer_name;
   int64_t _pixels_submitted{0};
   int32_t _draw_count{0};             //!< times the layer was drawn over the window, colour and normal pass each counting once
   int64_t _tiles_submitted{0};        //!< tiles handed to the rasteriser, before any per tile area
   int32_t _blocks_drawn{0};           //!< blocks this layer submitted over the window
   double _visible_fraction_sum{0.0};  //!< how much of each of them the view covered, summed
};

//! Per layer breakdown of the tile fill, accumulated across the report window rather than reset
//! every frame. Answers whether a handful of layers own the overdraw or it is spread evenly across
//! all of them, which is what decides between dropping layers and rejecting hidden fragments.
inline std::vector<TileMapLayerPixels> tilemap_layer_pixels;

///
/// \brief Starts attributing submitted tile pixels to one layer.
/// \param layer_name name of the tile map layer being drawn.
///
void beginTileMapLayer(const std::string& layer_name);

///
/// \brief Stops attributing submitted tile pixels to a layer.
///
void endTileMapLayer();

///
/// \brief Starts attributing submitted tile pixels to the normal pass.
///
void beginTileMapNormalPass();

///
/// \brief Stops attributing submitted tile pixels to the normal pass.
///
void endTileMapNormalPass();

///
/// \brief Returns the region of the view a draw through it is actually rasterised into.
/// \param view the view the draw goes through.
/// \return the view rectangle narrowed by the view's scissor, in view pixels.
/// \note the counter works in view space, so a pass clipped to part of the view would otherwise be
///       priced as the full screen pass it replaced. Folding the scissor in here is what makes the
///       occluder and normal clipping visible in the numbers.
///
sf::FloatRect getClipRectPx(const sf::View& view);

///
/// \brief Adds the on-screen area of the animated tiles submitted with a tile map batch.
/// \param view the view the batch was drawn through.
/// \param vertices first vertex of the animated run, two triangles per tile.
/// \param vertex_count how many vertices the run holds.
/// \note the animated tiles are gathered around the player block rather than around the view, so
///       unlike the static blocks they are not culled before submission. Clipping them here is what
///       keeps the layer totals comparable to each other.
///
void countAnimatedTilePixels(const sf::View& view, const sf::Vertex* vertices, std::size_t vertex_count);

///
/// \brief Adds the on-screen area of a batch of ambient occlusion quads.
/// \param target render target the batch went to.
/// \param states render states the batch was drawn with.
/// \param batched_vertices the batch itself, two triangles per quad.
///
void countAmbientOcclusionPixels(
   const sf::RenderTarget& target,
   const sf::RenderStates& states,
   const std::vector<sf::Vertex>& batched_vertices
);

///
/// \brief Adds the on-screen area of one image layer sprite.
/// \param target render target the sprite went to.
/// \param states render states the sprite was drawn with; carries the parallax view when there is one.
/// \param sprite the sprite that was drawn.
///
void countImageLayerPixels(const sf::RenderTarget& target, const sf::RenderStates& states, const sf::Sprite& sprite);
}  // namespace DrawCallCounter

#endif  // DEVELOPMENT_MODE
