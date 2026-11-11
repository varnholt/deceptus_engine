#include "debugdraw.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <span>

#include "framework/tools/localization.h"
#include "framework/tools/localizedtext.h"
#include "framework/tools/sfmlcompat.h"
#include "game/camera/camerasystem.h"
#include "game/physics/worldquery.h"

constexpr auto outline_thickness = 0.5f;

namespace
{
/// \brief submits a borrowed vertex range using the active backend's draw API.
/// \param target destination retaining its current view and render state.
/// \param vertices vertices consumed during the call without retaining ownership.
/// \param primitive primitive topology for the range.
void draw_vertices(sf::RenderTarget& target, std::span<const sf::Vertex> vertices, sf::PrimitiveType primitive)
{
#ifdef DECEPTUS_VRSFML
   target.draw(vertices, primitive);
#else
   target.draw(vertices.data(), vertices.size(), primitive);
#endif
}

/// \brief converts and draws a path with stack storage for ordinary box2d polygons.
/// \param target destination retaining its current view.
/// \param points borrowed box2d points consumed during the call.
/// \param offset world-space translation applied before pixel conversion.
/// \param color line color.
/// \param closed whether to connect the last point to the first.
void draw_path(sf::RenderTarget& target, std::span<const b2Vec2> points, const b2Vec2& offset, const sf::Color& color, bool closed)
{
   if (points.size() < 2)
   {
      return;
   }
   const auto count = points.size() + static_cast<size_t>(closed);
   std::array<sf::Vertex, b2_maxPolygonVertices + 1> local_vertices;
   std::vector<sf::Vertex> large_vertices;
   std::span<sf::Vertex> vertices{local_vertices};
   if (count > local_vertices.size())
   {
      large_vertices.resize(count);
      vertices = large_vertices;
   }
   for (size_t index = 0; index < points.size(); ++index)
   {
      vertices[index] = sf::Vertex(DebugDraw::vecB2S(points[index] + offset), color);
   }
   if (closed)
   {
      vertices[points.size()] = vertices.front();
   }
   draw_vertices(target, vertices.first(count), sf::PrimitiveType::LineStrip);
}
}  // namespace

sf::Color DebugDraw::glColorToSfml(const b2Color& color, uint8_t alpha)
{
   return sf::Color(static_cast<uint8_t>(color.r * 255), static_cast<uint8_t>(color.g * 255), static_cast<uint8_t>(color.b * 255), alpha);
}

sf::Vector2f DebugDraw::vecB2S(const b2Vec2& vector)
{
   return {vector.x * PPM, vector.y * PPM};
}

b2Vec2 DebugDraw::vecS2B(const sf::Vector2f& vector)
{
   return {vector.x * MPP, vector.y * MPP};
}

void DebugDraw::drawPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::ConvexShape polygon{sf::ConvexShape::Data{.pointCount = static_cast<std::size_t>(vertexCount)}};
#else
   sf::ConvexShape polygon(vertexCount);
#endif

   for (auto i = 0; i < vertexCount; i++)
   {
      polygon.setPoint(i, DebugDraw::vecB2S(vertices[i]));
   }

   polygon.setOutlineThickness(outline_thickness);
   polygon.setFillColor(sf::Color::Transparent);
   polygon.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(polygon);
}

void DebugDraw::drawSolidPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::ConvexShape polygon{sf::ConvexShape::Data{.pointCount = static_cast<std::size_t>(vertex_count)}};
#else
   sf::ConvexShape polygon(vertex_count);
#endif
   for (auto i = 0; i < vertex_count; i++)
   {
      sf::Vector2f transformedVec = DebugDraw::vecB2S(vertices[i]);
      polygon.setPoint(i, sf::Vector2f(std::floor(transformedVec.x), std::floor(transformedVec.y)));
   }

   polygon.setOutlineThickness(outline_thickness);
   polygon.setFillColor(DebugDraw::glColorToSfml(color, 60));
   polygon.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(polygon);
}

void DebugDraw::drawCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::CircleShape circle{sf::CircleShape::Data{.radius = radius * PPM}};
#else
   sf::CircleShape circle(radius * PPM);
#endif
   sfcompat::setOrigin(circle, {radius * PPM, radius * PPM});
   sfcompat::setPosition(circle, DebugDraw::vecB2S(center));
   circle.setFillColor(sf::Color::Transparent);
   circle.setOutlineThickness(outline_thickness);
   circle.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(circle);
}

void DebugDraw::drawCircle(sf::RenderTarget& target, const sf::Vector2f& center, float radius, const b2Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::CircleShape circle{sf::CircleShape::Data{.radius = radius}};
#else
   sf::CircleShape circle(radius);
#endif
   sfcompat::setOrigin(circle, {radius, radius});
   sfcompat::setPosition(circle, center);
   circle.setFillColor(sf::Color::Transparent);
   circle.setOutlineThickness(outline_thickness);
   circle.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(circle);
}

void DebugDraw::drawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::CircleShape circle{sf::CircleShape::Data{.radius = radius * PPM}};
#else
   sf::CircleShape circle(radius * PPM);
#endif
   sfcompat::setOrigin(circle, {radius * PPM, radius * PPM});
   sfcompat::setPosition(circle, DebugDraw::vecB2S(center));
   circle.setFillColor(DebugDraw::glColorToSfml(color, 255));
   circle.setOutlineThickness(1.f);
   circle.setOutlineColor(DebugDraw::glColorToSfml(color));

   const auto end_point = center + radius * axis;
   sf::Vertex line[2] = {
      sf::Vertex(DebugDraw::vecB2S(center), DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(end_point), DebugDraw::glColorToSfml(color)),
   };

   target.draw(circle);
#ifdef DECEPTUS_VRSFML
   target.draw(line, sf::PrimitiveType::Lines);
#else
   target.draw(line, 2, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawPoint(sf::RenderTarget& target, const sf::Vector2f& p, const b2Color& color)
{
   constexpr auto pointSize = 3;

   sf::Vertex line[] = {
      sf::Vertex(p + sf::Vector2f{-pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(p + sf::Vector2f{pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(p + sf::Vector2f{0, -pointSize}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(p + sf::Vector2f{0, pointSize}, DebugDraw::glColorToSfml(color))
   };

#ifdef DECEPTUS_VRSFML
   target.draw(line, sf::PrimitiveType::Lines);
#else
   target.draw(line, 4, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color)
{
   constexpr auto pointSize = 3;

   sf::Vertex line[] = {
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{-pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{0, -pointSize}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{0, pointSize}, DebugDraw::glColorToSfml(color))
   };

#ifdef DECEPTUS_VRSFML
   target.draw(line, sf::PrimitiveType::Lines);
#else
   target.draw(line, 4, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawLine(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
   sf::Vertex line[] = {
      sf::Vertex(DebugDraw::vecB2S(p1), DebugDraw::glColorToSfml(color)), sf::Vertex(DebugDraw::vecB2S(p2), DebugDraw::glColorToSfml(color))
   };

#ifdef DECEPTUS_VRSFML
   target.draw(line, sf::PrimitiveType::Lines);
#else
   target.draw(line, 2, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawLine(sf::RenderTarget& target, const sf::Vector2f& p1, const sf::Vector2f& p2, const b2Color& color)
{
   sf::Vertex line[] = {sf::Vertex(p1, DebugDraw::glColorToSfml(color)), sf::Vertex(p2, DebugDraw::glColorToSfml(color))};

#ifdef DECEPTUS_VRSFML
   target.draw(line, sf::PrimitiveType::Lines);
#else
   target.draw(line, 2, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawLines(sf::RenderTarget& target, const std::vector<b2Vec2>& lines, const b2Color& color)
{
   draw_path(target, lines, b2Vec2{0.0f, 0.0f}, glColorToSfml(color), false);
}

void DebugDraw::drawLines(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   const b2Vec2& offset,
   int32_t vertex_count,
   const b2Color& color
)
{
   if (!vertices || vertex_count < 2)
   {
      return;
   }
   draw_path(target, {vertices, static_cast<size_t>(vertex_count)}, offset, glColorToSfml(color), false);
}

void DebugDraw::drawLineLoop(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   const b2Vec2& offset,
   int32_t vertex_count,
   const b2Color& color
)
{
   if (!vertices || vertex_count < 2)
   {
      return;
   }
   draw_path(target, {vertices, static_cast<size_t>(vertex_count)}, offset, glColorToSfml(color), true);
}

void DebugDraw::drawTransform(sf::RenderTarget& target, const b2Transform& xf)
{
   constexpr auto line_length = 0.4f;

   const auto x_axis = xf.p + line_length * xf.q.GetXAxis();
   const auto y_axis = xf.p + line_length * xf.q.GetYAxis();

   const sf::Vertex line_red[] = {
      sf::Vertex(DebugDraw::vecB2S(xf.p), sf::Color::Red), sf::Vertex(DebugDraw::vecB2S(x_axis), sf::Color::Red)
   };

   const sf::Vertex line_green[] = {
      sf::Vertex(DebugDraw::vecB2S(xf.p), sf::Color::Green), sf::Vertex(DebugDraw::vecB2S(y_axis), sf::Color::Green)
   };

#ifdef DECEPTUS_VRSFML
   target.draw(line_red, sf::PrimitiveType::Lines);
   target.draw(line_green, sf::PrimitiveType::Lines);
#else
   target.draw(line_red, 2, sf::PrimitiveType::Lines);
   target.draw(line_green, 2, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::drawShape(sf::RenderTarget& target, sf::Shape& shape, const sf::Color& color, const sf::Color& fill_color, float thickness)
{
   shape.setOutlineThickness(thickness);
   shape.setFillColor(fill_color);
   shape.setOutlineColor(color);
   target.draw(shape);
}

void DebugDraw::drawRect(
   sf::RenderTarget& target,
   const sf::IntRect& rect,
   const sf::Color& color,
   const sf::Color& fill_color,
   float thickness
)
{
   drawRect(
      target,
      sf::FloatRect{
         {static_cast<float>(rect.position.x), static_cast<float>(rect.position.y)},
         {static_cast<float>(rect.size.x), static_cast<float>(rect.size.y)}
      },
      color,
      fill_color,
      thickness
   );
}

void DebugDraw::drawRect(
   sf::RenderTarget& target,
   const sf::FloatRect& rect,
   const sf::Color& color,
   const sf::Color& fill_color,
   float thickness
)
{
   // retain SFML's handling of degenerate rectangles and outlines that cross their center
   if (rect.size.x <= 0.0f || rect.size.y <= 0.0f || -2.0f * thickness >= std::min(rect.size.x, rect.size.y))
   {
#ifdef DECEPTUS_VRSFML
      sf::RectangleShape shape{sf::RectangleShape::Data{.size = rect.size}};
#else
      sf::RectangleShape shape(rect.size);
#endif
      sfcompat::setPosition(shape, rect.position);
      drawShape(target, shape, color, fill_color, thickness);
      return;
   }
   std::array<sf::Vertex, 30> vertices;
   size_t count = 0;
   const auto quad = [&](sf::Vector2f first, sf::Vector2f second, sf::Vector2f third, sf::Vector2f fourth, const sf::Color& tint)
   {
      vertices[count++] = sf::Vertex(first, tint);
      vertices[count++] = sf::Vertex(second, tint);
      vertices[count++] = sf::Vertex(third, tint);
      vertices[count++] = sf::Vertex(first, tint);
      vertices[count++] = sf::Vertex(third, tint);
      vertices[count++] = sf::Vertex(fourth, tint);
   };
   const auto left = rect.position.x;
   const auto top = rect.position.y;
   const auto right = left + rect.size.x;
   const auto bottom = top + rect.size.y;
   if (fill_color.a != 0)
   {
      quad({left, top}, {right, top}, {right, bottom}, {left, bottom}, fill_color);
   }
   if (thickness != 0.0f && color.a != 0)
   {
      const auto outer_left = left - thickness;
      const auto outer_top = top - thickness;
      const auto outer_right = right + thickness;
      const auto outer_bottom = bottom + thickness;
      quad({left, top}, {outer_left, outer_top}, {outer_right, outer_top}, {right, top}, color);
      quad({right, top}, {outer_right, outer_top}, {outer_right, outer_bottom}, {right, bottom}, color);
      quad({right, bottom}, {outer_right, outer_bottom}, {outer_left, outer_bottom}, {left, bottom}, color);
      quad({left, bottom}, {outer_left, outer_bottom}, {outer_left, outer_top}, {left, top}, color);
   }
   if (count != 0)
   {
      draw_vertices(target, std::span{vertices}.first(count), sf::PrimitiveType::Triangles);
   }
}

void DebugDraw::draw_label(
   sf::RenderTarget& target,
   const std::string& label,
   sf::Vector2f anchor,
   const sf::FloatRect& container,
   const sf::Color& color,
   const sf::Color& background,
   unsigned int character_size
)
{
   if (label.empty())
   {
      return;
   }
   auto text = sfcompat::createText(getFont());
   text.setString(LocalizedText::toSfmlString(label));
   text.setCharacterSize(character_size);
   text.setFillColor(color);
   const auto text_bounds = text.getLocalBounds();
   const sf::Vector2f padding{8.0f, 6.0f};
   const auto panel_size = text_bounds.size + padding * 2.0f;
   const sf::Vector2f panel_position{
      std::clamp(anchor.x, container.position.x, std::max(container.position.x, container.position.x + container.size.x - panel_size.x)),
      std::clamp(anchor.y, container.position.y, std::max(container.position.y, container.position.y + container.size.y - panel_size.y))
   };
   drawRect(target, sf::FloatRect{panel_position, panel_size}, background, background, 0.0f);
   sfcompat::setPosition(text, panel_position + padding - text_bounds.position);
   target.draw(text);
}

sf::FloatRect DebugDraw::getScreenRect(sf::RenderTarget& target)
{
#ifdef DECEPTUS_VRSFML
   const auto screen_view = target.computeView();

   const sf::FloatRect screen = {
      {screen_view.center.x - screen_view.size.x / 2.0f, screen_view.center.y - screen_view.size.y / 2.0f},
      {screen_view.size.x, screen_view.size.y}
   };
#else
   const auto& screen_view = target.getView();

   sf::FloatRect screen = {
      {screen_view.getCenter().x - screen_view.getSize().x / 2.0f, screen_view.getCenter().y - screen_view.getSize().y / 2.0f},
      {screen_view.getSize().x, screen_view.getSize().y}
   };
#endif

   return screen;
}

void DebugDraw::debugBodies(sf::RenderTarget& target, Level* level)
{
   for (auto* joint = level->getWorld()->GetJointList(); joint; joint = joint->GetNext())
   {
      auto distance_joint = dynamic_cast<b2DistanceJoint*>(joint);
      if (distance_joint)
      {
         drawLine(target, distance_joint->GetAnchorA(), distance_joint->GetAnchorB(), b2Color(1, 1, 0, 1));
      }
   }

   const auto screen = getScreenRect(target);

   // optimization is to only use bodies on screen by doing some world querying.
   // that reduces the amount of bodies to about 1/5, depending on the level complexity.
   //
   // for (
   //    auto body = level->getWorld()->GetBodyList();
   //    body != nullptr;
   //    body = body->GetNext()
   // )
   //

   auto bodies = WorldQuery::retrieveBodiesInsideRect(level->getWorld(), screen);

   for (auto* body : bodies)
   {
      if (body->GetType() == b2_dynamicBody || body->GetType() == b2_kinematicBody || body->GetType() == b2_staticBody)
      {
         // draw position and velocity
         static const b2Color point_color{1.0f, 1.0f, 0.0f, 1.0f};
         constexpr auto max_velocity = 5.0f;
         drawPoint(target, body->GetPosition(), point_color);

         b2Vec2 normalized_velocity{body->GetLinearVelocity()};
         const auto length = std::clamp(normalized_velocity.Normalize(), 0.0f, max_velocity);

         drawLine(target, body->GetPosition(), body->GetPosition() + normalized_velocity, b2Color(1.0f, length / max_velocity, 0.0, 1.0f));

         // draw fixtures
         auto* fixture = body->GetFixtureList();
         while (fixture)
         {
            auto next = fixture->GetNext();
            auto shape = fixture->GetShape();

            switch (shape->GetType())
            {
               case b2Shape::e_polygon:
               {
                  const auto offset = body->GetPosition();
                  auto poly = dynamic_cast<b2PolygonShape*>(shape);
                  drawLineLoop(target, poly->m_vertices, offset, poly->m_count, b2Color{1.0f, 0.3f, 0.0f, 1.0f});
                  break;
               }

               case b2Shape::e_circle:
               {
                  b2Vec2 offset{0.0f, 0.0f};
                  auto circle_shape = dynamic_cast<b2CircleShape*>(shape);

                  if (circle_shape)
                  {
                     offset = circle_shape->m_p;
                  }

                  drawCircle(target, body->GetPosition() + offset, shape->m_radius, b2Color{0.4f, 0.4f, 0.4f, 1.0f});
                  break;
               }

               case b2Shape::e_chain:
               {
                  const auto offset = body->GetPosition();
                  auto chain = dynamic_cast<b2ChainShape*>(shape);
                  drawLines(target, chain->m_vertices, offset, chain->m_count, b2Color{1.0f, 0.0f, 0.0f, 1.0f});
                  break;
               }

               default:
               {
                  break;
               }
            }

            fixture = next;
         }
      }
   }
}

void DebugDraw::debugCameraSystem(sf::RenderTarget& target)
{
   auto& camera_system = CameraSystem::getInstance();

   sf::Vertex f0[] = {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   sf::Vertex f1[] = {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   sf::Vertex p0[] = {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}}
   };

   sf::Vertex p1[] = {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}}
   };

#ifdef DECEPTUS_VRSFML
   target.draw(f0, sf::PrimitiveType::Lines);
   target.draw(f1, sf::PrimitiveType::Lines);
   target.draw(p0, sf::PrimitiveType::Lines);
   target.draw(p1, sf::PrimitiveType::Lines);
#else
   target.draw(f0, 2, sf::PrimitiveType::Lines);
   target.draw(f1, 2, sf::PrimitiveType::Lines);
   target.draw(p0, 2, sf::PrimitiveType::Lines);
   target.draw(p1, 2, sf::PrimitiveType::Lines);
#endif
}

void DebugDraw::debugHitboxes(sf::RenderTarget& target)
{
   using namespace std::chrono_literals;

   const auto screen = getScreenRect(target);
   const auto collided_nodes = WorldQuery::findNodesByHitbox(screen);

   const auto now = std::chrono::high_resolution_clock::now();

   for (const auto& collided_node : collided_nodes)
   {
      auto just_hit = false;
      const auto hit_time = collided_node._node->getHitTime();
      if (hit_time.has_value())
      {
         just_hit = (now - hit_time.value() < 500ms);
      }

      const auto fill_color = just_hit ? sf::Color{255, 0, 0, 50} : sf::Color{255, 255, 255, 50};

      for (const auto& hitbox : collided_node._node->_hitboxes)
      {
         drawRect(target, hitbox.getRectTranslated(), sf::Color::Magenta, fill_color);
      }
   }
}
