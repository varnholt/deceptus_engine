#include "box2dtools.h"

#include <algorithm>

b2PolygonShape Box2DTools::createBeveledBox(const float width, const float height, const float bevel_percentage)
{
   auto box_shape = b2PolygonShape();

   // ensure bevel percentage is within a valid range (0 to 50%)
   const auto clamped_bevel_percentage = std::clamp(bevel_percentage, 0.0f, 0.5f);

   // calculate the bevel offset based on the bevel percentage
   const auto bevel_offset = std::min(width, height) * clamped_bevel_percentage;

   //    0----7
   //   /      \
   //  1        6
   //  |        |
   //  2        5
   //   \      /
   //    3----4
   const auto vertices = std::array<b2Vec2, 8>{
      b2Vec2(bevel_offset, 0),
      b2Vec2(width - bevel_offset, 0),
      b2Vec2(width, bevel_offset),
      b2Vec2(width, height - bevel_offset),
      b2Vec2(width - bevel_offset, height),
      b2Vec2(bevel_offset, height),
      b2Vec2(0, height - bevel_offset),
      b2Vec2(0, bevel_offset)};

   box_shape.Set(vertices.data(), vertices.size());

   return box_shape;
}
