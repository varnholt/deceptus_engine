#pragma once

#ifdef DEVELOPMENT_MODE

#include <cstdint>

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

//! Candidates examined by Level::drawLayers while looking for things to draw at a z index. The loop
//! runs once per z index and rescans every container each time, so this grows with the z range
//! multiplied by the level's content rather than with what is actually on screen.
inline int32_t layer_scan_steps = 0;
}  // namespace DrawCallCounter

#endif  // DEVELOPMENT_MODE
