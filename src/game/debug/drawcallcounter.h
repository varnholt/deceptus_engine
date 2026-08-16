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
}

#endif  // DEVELOPMENT_MODE
