#pragma once

#include "pointf.h"

#include <cstdint>

/// \brief A vertex in the winged-edge planar graph, storing position and an outgoing edge index.
struct PathVertex
{
   explicit PathVertex(const PointF& point = PointF{}, int32_t edgeIndex = -1);
   operator PointF() const;

   int32_t edge = -1;  //!< Index of an outgoing edge from this vertex, or -1 if none.
   double x = 0.0;  //!< X coordinate.
   double y = 0.0;  //!< Y coordinate.
};

inline PathVertex::PathVertex(const PointF& point, int32_t edgeIndex) : edge(edgeIndex), x(point.x), y(point.y)
{
}

inline PathVertex::operator PointF() const
{
   return {x, y};
}
