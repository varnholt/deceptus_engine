#pragma once

#include "pointf.h"

namespace PathMerge
{

/// \brief A finite line segment defined by two endpoints.
struct LineF
{
   PointF p1;  //!< Start point.
   PointF p2;  //!< End point.
};

}  // namespace PathMerge
