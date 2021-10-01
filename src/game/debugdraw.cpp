#include "debugdraw.h"


static const auto outlineThickness = 0.5f;


//----------------------------------------------------------------------------------------------------------------------
sf::Color DebugDraw::GLColorToSFML(const b2Color& color, sf::Uint8 alpha)
{
  return sf::Color(
    static_cast<sf::Uint8>(color.r * 255),
    static_cast<sf::Uint8>(color.g * 255),
    static_cast<sf::Uint8>(color.b * 255),
    alpha
  );
}


//----------------------------------------------------------------------------------------------------------------------
sf::Vector2f DebugDraw::B2VecToSFVec(const b2Vec2 &vector)
{
   return sf::Vector2f(
      vector.x * PPM,
      vector.y * PPM
   );
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawPolygon(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   int32 vertexCount,
   const b2Color& color
)
{
   sf::ConvexShape polygon(vertexCount);

   for(int i = 0; i < vertexCount; i++)
   {
      sf::Vector2f transformedVec = DebugDraw::B2VecToSFVec(vertices[i]);

      polygon.setPoint(
         i,
         sf::Vector2f(
            transformedVec.x,
            transformedVec.y
         )
      );
   }

   polygon.setOutlineThickness(outlineThickness);
   polygon.setFillColor(sf::Color::Transparent);
   polygon.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(polygon);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawSolidPolygon(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   int32 vertex_count,
   const b2Color& color
)
{
   sf::ConvexShape polygon(vertex_count);
   for(int i = 0; i < vertex_count; i++)
   {
      sf::Vector2f transformedVec = DebugDraw::B2VecToSFVec(vertices[i]);
      polygon.setPoint(i, sf::Vector2f(std::floor(transformedVec.x), std::floor(transformedVec.y)));
   }

   polygon.setOutlineThickness(outlineThickness);
   polygon.setFillColor(DebugDraw::GLColorToSFML(color, 60));
   polygon.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(polygon);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawCircle(
   sf::RenderTarget& target,
   const b2Vec2& center,
   float32 radius,
   const b2Color& color
)
{
   sf::CircleShape circle(radius * PPM);
   circle.setOrigin(radius * PPM, radius * PPM);
   circle.setPosition(DebugDraw::B2VecToSFVec(center));
   circle.setFillColor(sf::Color::Transparent);
   circle.setOutlineThickness(outlineThickness);
   circle.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(circle);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
   sf::CircleShape circle(radius * PPM);
   circle.setOrigin(radius * PPM, radius * PPM);
   circle.setPosition(DebugDraw::B2VecToSFVec(center));
   circle.setFillColor(DebugDraw::GLColorToSFML(color, 255));
   circle.setOutlineThickness(1.f);
   circle.setOutlineColor(DebugDraw::GLColorToSFML(color));

   b2Vec2 end_point = center + radius * axis;
   sf::Vertex line[2] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(center), DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(end_point), DebugDraw::GLColorToSFML(color)),
   };

   target.draw(circle);
   target.draw(line, 2, sf::Lines);
}

//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color)
{
   static const auto pointSize = 3;

   sf::Vertex line[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(p) + sf::Vector2f{- pointSize, 0}, DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(p) + sf::Vector2f{  pointSize, 0}, DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(p) + sf::Vector2f{0, - pointSize}, DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(p) + sf::Vector2f{0,   pointSize}, DebugDraw::GLColorToSFML(color))
   };

   target.draw(line, 4, sf::Lines);
}

//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawSegment(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
   sf::Vertex line[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(p1), DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(p2), DebugDraw::GLColorToSFML(color))
   };

   target.draw(line, 2, sf::Lines);
}



//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::DrawTransform(sf::RenderTarget& target, const b2Transform& xf)
{
   float line_length = 0.4f;

   b2Vec2 x_axis = xf.p + line_length * xf.q.GetXAxis();
   sf::Vertex redLine[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(xf.p), sf::Color::Red),
      sf::Vertex(DebugDraw::B2VecToSFVec(x_axis), sf::Color::Red)
   };

   b2Vec2 y_axis = xf.p + line_length * xf.q.GetYAxis();
   sf::Vertex greenLine[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(xf.p), sf::Color::Green),
      sf::Vertex(DebugDraw::B2VecToSFVec(y_axis), sf::Color::Green)
   };

   target.draw(redLine, 2, sf::Lines);
   target.draw(greenLine, 2, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawShape(sf::RenderTarget& target, sf::Shape& shape, const sf::Color& color)
{
   shape.setOutlineThickness(outlineThickness);
   shape.setFillColor(sf::Color::Transparent);
   shape.setOutlineColor(color);
   target.draw(shape);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawRect(sf::RenderTarget& target, const sf::IntRect& rect, const sf::Color& color)
{
   sf::RectangleShape rs;
   auto pos = sf::Vector2{static_cast<float>(rect.left), static_cast<float>(rect.top)};
   auto size = sf::Vector2f{static_cast<float>(rect.width), static_cast<float>(rect.height)};
   rs.setSize(size);
   rs.setPosition(pos);
   drawShape(target, rs, color);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawRect(sf::RenderTarget& target, const sf::FloatRect& rect, const sf::Color& color)
{
   sf::RectangleShape rs;
   auto pos = sf::Vector2{static_cast<float>(rect.left), static_cast<float>(rect.top)};
   auto size = sf::Vector2f{static_cast<float>(rect.width), static_cast<float>(rect.height)};
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
         DrawSegment(
            target,
            distance_joint->GetAnchorA(),
            distance_joint->GetAnchorB(),
            b2Color(1, 1, 0, 1)
         );
      }
   }

   for (
      auto body = level->getWorld()->GetBodyList();
      body != nullptr;
      body = body->GetNext()
   )
   {
      if (
            body->GetType() == b2_dynamicBody
         || body->GetType() == b2_kinematicBody
#ifdef RELEASE_BUILD
         || body->GetType() == b2_staticBody
#endif
      )
      {

         // draw position and velocity
         static const b2Color point_color{1.0f, 1.0f, 0.0f, 1.0f};
         static const auto max_velocity = 5.0f;
         DrawPoint(target, body->GetPosition(), point_color);

         b2Vec2 normalized_velocity{body->GetLinearVelocity()};
         const auto length = std::clamp(normalized_velocity.Normalize(), 0.0f, max_velocity);

         DrawSegment(
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

                  auto vertexCount = poly->GetVertexCount();

                  // to debug static bodies
                  //
                  // if (vertexCount > 100)
                  // {
                  //    break;
                  // }

                  auto vertices = new b2Vec2[static_cast<size_t>(vertexCount)];

                  for (auto i = 0; i < vertexCount; i++ )
                  {
                     auto vec2 = poly->GetVertex(i);
                     vertices[i] = vec2;
                     vertices[i].x += body->GetPosition().x;
                     vertices[i].y += body->GetPosition().y;
                  }

                  DrawPolygon(
                     target,
                     vertices,
                     vertexCount,
                     b2Color(1,0,0,1)
                  );

                  delete[] vertices;
                  break;
               }

               case b2Shape::e_circle:
               {
                  b2Vec2 offset{0.0f, 0.0f};
                  b2CircleShape* circleShape = nullptr;
                  circleShape = dynamic_cast<b2CircleShape*>(f->GetShape());
                  if (circleShape != nullptr)
                  {
                     offset = circleShape->m_p;
                  }

                  DrawCircle(
                     target,
                     body->GetPosition() + offset,
                     shape->m_radius,
                     b2Color(0.4f, 0.4f, 0.4f, 1.0f)
                  );
                  break;
               }

               case b2Shape::e_chain:
               {
                  auto chain = dynamic_cast<b2ChainShape*>(shape);

                  auto vertexCount = chain->m_count;

                  // to debug static bodies
                  //
                  // if (vertexCount > 100)
                  // {
                  //    break;
                  // }

                  auto vertices = new b2Vec2[static_cast<size_t>(vertexCount)];

                  for(auto i = 0; i < vertexCount; i++ )
                  {
                     auto vec2 = chain->m_vertices[i];
                     vertices[i] = vec2;
                     vertices[i].x += body->GetPosition().x;
                     vertices[i].y += body->GetPosition().y;
                  }

                  DrawPolygon(
                     target,
                     vertices,
                     vertexCount,
                     b2Color(1,0,0,1)
                  );

                  delete[] vertices;
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
            DrawPolygon(
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

   sf::Vertex f0[2] =
   {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX0(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   target.draw(f0, 2, sf::Lines);

   sf::Vertex f1[2] =
   {
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{camera_system.getFocusZoneX1(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   target.draw(f1, 2, sf::Lines);

   sf::Vertex p0[2] =
   {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY0()}, sf::Color{0, 50, 255, 100}}
   };

   target.draw(p0, 2, sf::Lines);

   sf::Vertex p1[2] =
   {
      sf::Vertex{sf::Vector2f{0.0f, camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), camera_system.getPanicLineY1()}, sf::Color{0, 50, 255, 100}}
   };

   target.draw(p1, 2, sf::Lines);
}

