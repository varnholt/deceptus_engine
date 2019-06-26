#include "debugdraw.h"


sf::Color DebugDraw::GLColorToSFML(const b2Color& color, sf::Uint8 alpha)
{
  return sf::Color(
    static_cast<sf::Uint8>(color.r * 255),
    static_cast<sf::Uint8>(color.g * 255),
    static_cast<sf::Uint8>(color.b * 255),
    alpha
  );
}


sf::Vector2f DebugDraw::B2VecToSFVec(const b2Vec2 &vector)
{
   return sf::Vector2f(
      vector.x * PPM,
      vector.y * PPM
   );
}


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

   polygon.setOutlineThickness(-0.3f);
   polygon.setFillColor(sf::Color::Transparent);
   polygon.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(polygon);
}


void DebugDraw::DrawSolidPolygon(
   sf::RenderTarget& target,
   const b2Vec2* vertices,
   int32 vertexCount,
   const b2Color& color
)
{
   sf::ConvexShape polygon(vertexCount);
   for(int i = 0; i < vertexCount; i++)
   {
      //polygon.setPoint(i, SFMLDraw::B2VecToSFVec(vertices[i]));
      sf::Vector2f transformedVec = DebugDraw::B2VecToSFVec(vertices[i]);
      polygon.setPoint(i, sf::Vector2f(std::floor(transformedVec.x), std::floor(transformedVec.y)));
      // flooring the coords to fix distorted lines on flat surfaces
   }

   // they still show up though.. but less frequently
   polygon.setOutlineThickness(-0.3f);
   polygon.setFillColor(DebugDraw::GLColorToSFML(color, 60));
   polygon.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(polygon);
}


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
   circle.setOutlineThickness(-0.3f);
   circle.setOutlineColor(DebugDraw::GLColorToSFML(color));

   target.draw(circle);
}


void DebugDraw::DrawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
   sf::CircleShape circle(radius * PPM);
   circle.setOrigin(radius * PPM, radius * PPM);
   circle.setPosition(DebugDraw::B2VecToSFVec(center));
   circle.setFillColor(DebugDraw::GLColorToSFML(color, 255 /*60*/));
   circle.setOutlineThickness(1.f);
   circle.setOutlineColor(DebugDraw::GLColorToSFML(color));

   b2Vec2 endPoint = center + radius * axis;
   sf::Vertex line[2] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(center), DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(endPoint), DebugDraw::GLColorToSFML(color)),
   };

   target.draw(circle);
   target.draw(line, 2, sf::Lines);
}



void DebugDraw::DrawSegment(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
   sf::Vertex line[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(p1), DebugDraw::GLColorToSFML(color)),
      sf::Vertex(DebugDraw::B2VecToSFVec(p2), DebugDraw::GLColorToSFML(color))
   };

   target.draw(line, 2, sf::Lines);
}



void DebugDraw::DrawTransform(sf::RenderTarget& target, const b2Transform& xf)
{
   float lineLength = 0.4f;

   /*b2Vec2 xAxis(b2Vec2(xf.p.x + (lineLength * xf.q.c), xf.p.y + (lineLength * xf.q.s)));*/
   b2Vec2 xAxis = xf.p + lineLength * xf.q.GetXAxis();
   sf::Vertex redLine[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(xf.p), sf::Color::Red),
      sf::Vertex(DebugDraw::B2VecToSFVec(xAxis), sf::Color::Red)
   };

   // You might notice that the ordinate(Y axis) points downward unlike the one in Box2D testbed
   // That's because the ordinate in SFML coordinate system points downward while the OpenGL(testbed) points upward
   /*b2Vec2 yAxis(b2Vec2(xf.p.x + (lineLength * -xf.q.s), xf.p.y + (lineLength * xf.q.c)));*/
   b2Vec2 yAxis = xf.p + lineLength * xf.q.GetYAxis();
   sf::Vertex greenLine[] =
   {
      sf::Vertex(DebugDraw::B2VecToSFVec(xf.p), sf::Color::Green),
      sf::Vertex(DebugDraw::B2VecToSFVec(yAxis), sf::Color::Green)
   };

   target.draw(redLine, 2, sf::Lines);
   target.draw(greenLine, 2, sf::Lines);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawShape(sf::RenderTarget& target, sf::Shape& shape)
{
   shape.setOutlineThickness(-0.3f);
   shape.setFillColor(sf::Color::Transparent);
   shape.setOutlineColor(sf::Color::Red);
   target.draw(shape);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::drawRect(sf::RenderTarget& target, const sf::Rect<int32_t>& rect)
{
   sf::RectangleShape rs;
   auto pos = sf::Vector2{static_cast<float>(rect.left), static_cast<float>(rect.top)};
   auto size = sf::Vector2f{static_cast<float>(rect.width), static_cast<float>(rect.height)};
   rs.setSize(size);
   rs.setPosition(pos);
   drawShape(target, rs);
}


//----------------------------------------------------------------------------------------------------------------------
void DebugDraw::debugBodies(sf::RenderTarget& target, Level* level)
{
   for (auto joint = level->getWorld()->GetJointList(); joint != nullptr; joint = joint->GetNext())
   {
      auto distanceJoint = dynamic_cast<b2DistanceJoint*>(joint);
      if (distanceJoint != nullptr)
      {
         DrawSegment(
            target,
            distanceJoint->GetAnchorA(),
            distanceJoint->GetAnchorB(),
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
      )
      {
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
                  auto vertices = new b2Vec2[static_cast<size_t>(vertexCount)];

                  for(auto i = 0; i < vertexCount; i++ )
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
                  b2Vec2 offset;
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
         auto vtxIt = level->getPointMap()->find(body);
         auto vtxCountIt = level->getPointSizeMap()->find(body);

         if (
               vtxIt != level->getPointMap()->end()
            && vtxCountIt != level->getPointSizeMap()->end()
         )
         {
            DrawPolygon(
               target,
               vtxIt->second,
               vtxCountIt->second,
               b2Color(1,0,0)
            );
         }
      }
   }
}


void DebugDraw::debugCameraSystem(sf::RenderTarget& target)
{
   auto& cameraSystem = CameraSystem::getCameraSystem();

   sf::Vertex f0[2] =
   {
      sf::Vertex{sf::Vector2f{cameraSystem.getFocusZoneX0(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{cameraSystem.getFocusZoneX0(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   target.draw(f0, 2, sf::Lines);

   sf::Vertex f1[2] =
   {
      sf::Vertex{sf::Vector2f{cameraSystem.getFocusZoneX1(), 0.0f}, sf::Color{255, 0, 0, 100}},
      sf::Vertex{sf::Vector2f{cameraSystem.getFocusZoneX1(), static_cast<float>(target.getSize().y)}, sf::Color{255, 0, 0, 100}}
   };

   target.draw(f1, 2, sf::Lines);

   sf::Vertex p0[2] =
   {
      sf::Vertex{sf::Vector2f{0.0f, cameraSystem.getPanicLineY0()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), cameraSystem.getPanicLineY0()}, sf::Color{0, 50, 255, 100}}
   };

   target.draw(p0, 2, sf::Lines);

   sf::Vertex p1[2] =
   {
      sf::Vertex{sf::Vector2f{0.0f, cameraSystem.getPanicLineY1()}, sf::Color{0, 50, 255, 100}},
      sf::Vertex{sf::Vector2f{static_cast<float>(target.getSize().x), cameraSystem.getPanicLineY1()}, sf::Color{0, 50, 255, 100}}
   };

   target.draw(p1, 2, sf::Lines);
}

