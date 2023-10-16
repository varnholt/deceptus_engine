#include "box2dtools.h"

#include <algorithm>

b2PolygonShape Box2DTools::createBeveledBox(const float width, const float height, const float bevel_percentage)
{
   auto box_shape = b2PolygonShape();

   // Ensure bevel percentage is within a valid range (0 to 50%)
   const auto clamped_bevel_percentage = std::clamp(bevel_percentage, 0.0f, 0.5f);

   // Calculate the bevel offset based on the bevel percentage
   const auto bevel_offset = std::min(width, height) * clamped_bevel_percentage;

   // Define the vertices of the beveled box
   const auto half_width = width * 0.5f;
   const auto half_height = height * 0.5f;
   const auto vertices = std::array<b2Vec2, 8>{
      b2Vec2(-half_width, -half_height),
      b2Vec2(half_width, -half_height),
      b2Vec2(half_width - bevel_offset, -half_height + bevel_offset),
      b2Vec2(-half_width + bevel_offset, -half_height + bevel_offset),
      b2Vec2(-half_width, half_height),
      b2Vec2(half_width, half_height),
      b2Vec2(half_width - bevel_offset, half_height - bevel_offset),
      b2Vec2(-half_width + bevel_offset, half_height - bevel_offset)};

   // Set the vertices in counter-clockwise order
   box_shape.Set(vertices.data(), vertices.size());

   return box_shape;
}
