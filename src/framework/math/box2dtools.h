#ifndef BOX2DTOOLS_H
#define BOX2DTOOLS_H

#include <array>
#include "box2d/box2d.h"

namespace Box2DTools
{
///
/// \brief Creates an 8-vertex beveled rectangle polygon for Box2D.
/// \param width Shape width.
/// \param height Shape height.
/// \param bevel_percentage Corner bevel amount clamped to [0.0, 0.5].
/// \return Box2D polygon shape with beveled corners.
///
b2PolygonShape createBeveledBox(const float width, const float height, const float bevel_percentage);
};  // namespace Box2DTools

#endif  // BOX2DTOOLS_H
