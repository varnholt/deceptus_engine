#include "debugdraw.h"

#include <iostream>

#include "framework/tools/elapsedtimer.h"
#include "worldquery.h"


static constexpr auto outline_thickness = 0.5f;


//----------------------------------------------------------------------------------------------------------------------
sf::Color DebugDraw::glColorToSfml(const b2Color& color, sf::Uint8 alpha)
{
  return sf::Color(
    static_cast<sf::Uint8>(color.r * 255),
    static_cast<sf::Uint8>(color.g * 255),
    static_cast<sf::Uint8>(color.b * 255),
    alpha
  );
}


//----------------------------------------------------------------------------------------------------------------------
sf::Vector2f DebugDraw::vecB2S(const b2Vec2 &vector)
{
   return{vector.x * PPM, vector.y * PPM};
}


//----------------------------------------------------------------------------------------------------------------------
b2Vec2 DebugDraw::vecS2B(const sf::Vector2f& vector)
{
   return{vector.x * MPP, vector.y * MPP};
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawPolygon(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   int32 vertexCount,
   const b2Color& color
)
{
   sf::ConvexShape polygon(vertexCount);

   for (auto i = 0; i < vertexCount; i++)
   {
      polygon.setPoint(i, DebugDraw::vecB2S(vertices[i]));
   }

   polygon.setOutlineThickness(outline_thickness);
   polygon.setFillColor(sf::Color::Transparent);
   polygon.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(polygon);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawSolidPolygon(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   int32 vertex_count,
   const b2Color& color
)
{
   sf::ConvexShape polygon(vertex_count);
   for(auto i = 0; i < vertex_count; i++)
   {
      sf::Vector2f transformedVec = DebugDraw::vecB2S(vertices[i]);
      polygon.setPoint(i, sf::Vector2f(std::floor(transformedVec.x), std::floor(transformedVec.y)));
   }

   polygon.setOutlineThickness(outline_thickness);
   polygon.setFillColor(DebugDraw::glColorToSfml(color, 60));
   polygon.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(polygon);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawCircle(
   sf::RenderTarget& target,
   const b2Vec2& center,
   float32 radius,
   const b2Color& color
)
{
   sf::CircleShape circle(radius * PPM);
   circle.setOrigin(radius * PPM, radius * PPM);
   circle.setPosition(DebugDraw::vecB2S(center));
   circle.setFillColor(sf::Color::Transparent);
   circle.setOutlineThickness(outline_thickness);
   circle.setOutlineColor(DebugDraw::glColorToSfml(color));

   target.draw(circle);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
   sf::CircleShape circle(radius * PPM);
   circle.setOrigin(radius * PPM, radius * PPM);
   circle.setPosition(DebugDraw::vecB2S(center));
   circle.setFillColor(DebugDraw::glColorToSfml(color, 255));
   circle.setOutlineThickness(1.f);
   circle.setOutlineColor(DebugDraw::glColorToSfml(color));

   const auto end_point = center + radius * axis;
   sf::Vertex line[2] =
   {
      sf::Vertex(DebugDraw::vecB2S(center), DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(end_point), DebugDraw::glColorToSfml(color)),
   };

   target.draw(circle);
   target.draw(line, 2, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color)
{
   static constexpr auto pointSize = 3;

   sf::Vertex line[] =
   {
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{- pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{  pointSize, 0}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{0, - pointSize}, DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p) + sf::Vector2f{0,   pointSize}, DebugDraw::glColorToSfml(color))
   };

   target.draw(line, 4, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawLine(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
   sf::Vertex line[] =
   {
      sf::Vertex(DebugDraw::vecB2S(p1), DebugDraw::glColorToSfml(color)),
      sf::Vertex(DebugDraw::vecB2S(p2), DebugDraw::glColorToSfml(color))
   };

   target.draw(line, 2, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawLines(sf::RenderTarget& target, const std::vector<b2Vec2>& lines, const b2Color& color)
{
   const auto sf_color = DebugDraw::glColorToSfml(color);
   std::vector<sf::Vertex> sf_lines;

   std::transform(lines.begin(), lines.end(), sf_lines.begin(), [sf_color](const auto& val)
   {
      return sf::Vertex(vecB2S(val), sf_color);
   });

   target.draw(sf_lines.data(), sf_lines.size(), sf::LineStrip);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawLines(sf::RenderTarget& target, const b2Vec2* vertices, int32_t vertex_count, const b2Color& color)
{
   const auto sf_color = DebugDraw::glColorToSfml(color);
   std::vector<sf::Vertex> sf_lines;

   for (auto i = 0; i < vertex_count; i++)
   {
      sf_lines.push_back(sf::Vertex(vecB2S(vertices[i]), sf_color));
   }

   target.draw(sf_lines.data(), sf_lines.size(), sf::LineStrip);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawTransform(sf::RenderTarget& target, const b2Transform& xf)
{
   static constexpr auto line_length = 0.4f;

   const auto x_axis = xf.p + line_length * xf.q.GetXAxis();
   const auto y_axis = xf.p + line_length * xf.q.GetYAxis();

   const sf::Vertex line_red[] =
   {
      sf::Vertex(DebugDraw::vecB2S(xf.p), sf::Color::Red),
      sf::Vertex(DebugDraw::vecB2S(x_axis), sf::Color::Red)
   };

   const sf::Vertex line_green[] =
   {
      sf::Vertex(DebugDraw::vecB2S(xf.p), sf::Color::Green),
      sf::Vertex(DebugDraw::vecB2S(y_axis), sf::Color::Green)
   };

   target.draw(line_red, 2, sf::Lines);
   target.draw(line_green, 2, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawShape(sf::RenderTarget& target, sf::Shape& shape, const sf::Color& color)
{
   shape.setOutlineThickness(outline_thickness);
   shape.setFillColor(sf::Color::Transparent);
   shape.setOutlineColor(color);
   target.draw(shape);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawRect(sf::RenderTarget& target, const sf::IntRect& rect, const sf::Color& color)
{
   sf::RectangleShape rs;
   const auto pos = sf::Vector2{static_cast<float>(rect.left), static_cast<float>(rect.top)};
   const auto size = sf::Vector2f{static_cast<float>(rect.width), static_cast<float>(rect.height)};
   rs.setSize(size);
   rs.setPosition(pos);
   drawShape(target, rs, color);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawRect(sf::RenderTarget& target, const sf::FloatRect& rect, const sf::Color& color)
{
   sf::RectangleShape rs;
   const auto pos = sf::Vector2{static_cast<float>(rect.left), static_cast<float>(rect.top)};
   const auto size = sf::Vector2f{static_cast<float>(rect.width), static_cast<float>(rect.height)};
   rs.setSize(size);
   rs.setPosition(pos);
   drawShape(target, rs, color);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::debugBodies(sf::RenderTarget& target, Level* level)
{
   for (auto joint = level->getWorld()->GetJointList(); joint != nullptr; joint = joint->GetNext())
   {
      auto distance_joint = dynamic_cast<b2DistanceJoint*>(joint);
      if (distance_joint != nullptr)
      {
         drawLine(
            target,
            distance_joint->GetAnchorA(),
            distance_joint->GetAnchorB(),
            b2Color(1, 1, 0, 1)
         );
      }
   }

   const auto& screen_view = target.getView();

   sf::FloatRect screen = {
      screen_view.getCenter().x - screen_view.getSize().x / 2.0f,
      screen_view.getCenter().y - screen_view.getSize().y / 2.0f,
      screen_view.getSize().x,
      screen_view.getSize().y
   };

   // optimization is to only use bodies on screen by doing some world querying.
   // that reduces the amount of bodies to about 1/5, depending on the level complexity.
   //
   // std::cout << "bodies on screen " << bodies.size() << " / " << level->getWorld()->GetBodyCount() << std::endl;
   //
   // for (
   //    auto body = level->getWorld()->GetBodyList();
   //    body != nullptr;
   //    body = body->GetNext()
   // )
   //

   std::vector<b2Body*> bodies = retrieveBodiesOnScreen(level->getWorld(), screen);

   for (auto body : bodies)
   {
      if (
            body->GetType() == b2_dynamicBody
         || body->GetType() == b2_kinematicBody
         || body->GetType() == b2_staticBody
      )
      {

         // draw position and velocity
         static const b2Color point_color{1.0f, 1.0f, 0.0f, 1.0f};
         static constexpr auto max_velocity = 5.0f;
         drawPoint(target, body->GetPosition(), point_color);

         b2Vec2 normalized_velocity{body->GetLinearVelocity()};
         const auto length = std::clamp(normalized_velocity.Normalize(), 0.0f, max_velocity);

         drawLine(
            target,
            body->GetPosition(),
            body->GetPosition() + normalized_velocity,
            b2Color(1.0f, length / max_velocity, 0.0, 1.0f)
         );

         // draw fixtures
         auto f = body->GetFixtureList();
         while (f)
         {
            auto next = f->GetNext();
            auto shape = f->GetShape();

            switch (shape->GetType())
            {
               case b2Shape::e_polygon:
               {
                  auto poly = dynamic_cast<b2PolygonShape*>(shape);
                  drawLines(target, poly->m_vertices, poly->m_count, b2Color{1, 0, 0, 1});
                  break;
               }

               case b2Shape::e_circle:
               {
                  b2Vec2 offset{0.0f, 0.0f};
                  auto circle_shape = dynamic_cast<b2CircleShape*>(f->GetShape());

                  if (circle_shape != nullptr)
                  {
                     offset = circle_shape->m_p;
                  }

                  drawCircle(
                     target,
                     body->GetPosition() + offset,
                     shape->m_radius,
                     b2Color{0.4f, 0.4f, 0.4f, 1.0f}
                  );
                  break;
               }

               case b2Shape::e_chain:
               {
                  auto chain = dynamic_cast<b2ChainShape*>(shape);
                  drawLines(target, chain->m_vertices, chain->m_count, b2Color{1, 0, 0, 1});
                  break;
               }

               default:
               {
                  break;
               }
            }

            f = next;
         }
      }
      else
      {
         const auto& vertex_it = level->getPointMap().find(body);
         const auto& vertex_count_it = level->getPointSizeMap().find(body);

         if (
               vertex_it != level->getPointMap().end()
            && vertex_count_it != level->getPointSizeMap().end()
         )
         {
            drawLines(
               target,
               vertex_it->second,
               static_cast<int32_t>(vertex_count_it->second),
               b2Color(1.0f, 0.0f, 0.0f)
            );
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::debugCameraSystem(sf::RenderTarget& target)
{
   auto& camera_system = CameraSystem::getCameraSystem();

   sf::Vertex f0[] =
   {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   sf::Vertex f1[] =
   {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   sf::Vertex p0[] =
   {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}}
   };

   sf::Vertex p1[] =
   {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}}
   };

   target.draw(f0, 2, sf::Lines);
   target.draw(f1, 2, sf::Lines);
   target.draw(p0, 2, sf::Lines);
   target.draw(p1, 2, sf::Lines);
}


std::vector<b2Body*> DebugDraw::retrieveBodiesOnScreen(const std::shared_ptr<b2World>& world, const sf::FloatRect& screen)
{
   b2AABB aabb;

   const auto l = screen.left;
   const auto r = screen.left + screen.width;
   const auto t = screen.top;
   const auto b = screen.top + screen.height;

   aabb.upperBound = vecS2B({std::max(l, r), std::max(b, t)});
   aabb.lowerBound = vecS2B({std::min(l, r), std::min(b, t)});

   return WorldQuery::queryBodies(world, aabb);
}

