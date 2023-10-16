#ifndef BOX2DTOOLS_H
#define BOX2DTOOLS_H

#include <array>
#include "box2d/box2d.h"

namespace Box2DTools
{
b2PolygonShape createBeveledBox(const float width, const float height, const float bevel_percentage);
};  // namespace Box2DTools

#endif  // BOX2DTOOLS_H
