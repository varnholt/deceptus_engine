#include "raycastlight.h"

#include "game/debugdraw.h"
#include "game/level.h"
#include "game/player.h"

#include <iostream>

#include <SFML/OpenGL.hpp>

#include <math.h>



//-----------------------------------------------------------------------------
const float step = (2.0f * static_cast<float>(M_PI)) / 360.0f;

// http://www.iforce2d.net/b2dtut/raycasting



//-----------------------------------------------------------------------------
RaycastLight::RaycastLight()
 : Effect("raycast light")
{
}



//-----------------------------------------------------------------------------
bool RaycastLight::onLoad()
{
//   if (!mShader.loadFromFile("data/shaders/raycast.vert", "data/shaders/raycast.frag"))
//   {
//      return false;
//   }

   return true;
}


//-----------------------------------------------------------------------------
void RaycastLight::onUpdate(float /*time*/, float /*x*/, float /*y*/)
{
   return;

   // box2d raycasting kicked out, too slow

   b2Vec2 p1;
   b2Vec2 p2;

   for (auto light : mLights)
   {
      light->mIntersections.clear();

      // printf("light pos in meters: %f, %f\n", light->mPos.x, light->mPos.y);

      for (auto angle = 0.0f; angle < 2.0f * M_PI; angle += step)
      {
         p1 = light->mPosMeters;
         //         p1.x = light->mPos.x + sin(angle) * 1.0f;
         //         p1.y = light->mPos.y + cos(angle) * 1.0f;

         p2.x = light->mPosMeters.x + sin(angle) * 20.0f;
         p2.y = light->mPosMeters.y + cos(angle) * 20.0f;

         // set up input
         b2RayCastInput input;
         input.p1 = p1;
         input.p2 = p2;
         input.maxFraction = 1;

         // check every fixture of every body to find closest
         float closestFraction = 1.0f;

         // b2Vec2 intersectionNormal(0, 0);
         for (b2Body* b = Level::getCurrentLevel()->getWorld()->GetBodyList(); b; b = b->GetNext())
         {
            if (b == Player::getPlayer(0)->getBody())
               continue;

            for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
            {
               auto shape = f->GetShape();
               auto chainShape = dynamic_cast<b2ChainShape*>(shape);
               b2RayCastOutput output;

               if (chainShape)
               {
                  for (auto vi = 0; vi < chainShape->m_count; vi++)
                  {
                     auto dist = chainShape->m_vertices[vi] - p1;
                     auto len = dist.x * dist.x + dist.y * dist.y;
                     if (len > light->maxDist)
                        continue;

                     if (f->RayCast(&output, input, vi))
                     {
                        if (output.fraction < closestFraction)
                        {
                           closestFraction = output.fraction;
                           // intersectionNormal = output.normal;
                        }
                     }
                  }
               }
               else
               {
                  if (f->RayCast(&output, input, 0))
                  {
                     if (output.fraction < closestFraction)
                     {
                        closestFraction = output.fraction;
                        // intersectionNormal = output.normal;
                     }
                  }
               }
            }
            // printf("close: %f\n", closestFraction);
         }

         b2Vec2 intersectionPoint = p1 + closestFraction * (p2 - p1);
         light->mIntersections.push_back(intersectionPoint);

         // printf("intersect pos %zd in meters: %f, %f\n", light->mIntersections.size(), intersectionPoint.x, intersectionPoint.y);
      }
   }

   // auto lightPos = mLights.at(0)->mPos;
   // mShader.setUniform("light1", sf::Vector2f(lightPos.x* PPM, lightPos.y* PPM));
}


//-----------------------------------------------------------------------------
void RaycastLight::drawLines(sf::RenderTarget& target, std::shared_ptr<LightInstance> light) const
{
   auto p1 = light->mPosMeters;
   for (auto p2 : light->mIntersections)
   {
      sf::Vertex line[] =
      {
         sf::Vertex(sf::Vector2f(p1.x, p1.y) * PPM, light->mColor),
         sf::Vertex(sf::Vector2f(p2.x, p2.y) * PPM, light->mColor)
      };

      target.draw(line, 2, sf::Lines);
   }
}


//-----------------------------------------------------------------------------
void RaycastLight::drawTriangles(sf::RenderTarget& target, std::shared_ptr<LightInstance> light) const
{
   auto p1 = light->mPosMeters;

   for (auto i = 0; i < light->mIntersections.size() - 1; i++)
   {
      auto p2 = light->mIntersections[i];
      auto p3 = light->mIntersections[i + 1];

      sf::Vertex tri[] =
      {
         sf::Vertex(sf::Vector2f(p1.x, p1.y) * PPM, light->mColor),
         sf::Vertex(sf::Vector2f(p2.x, p2.y) * PPM, light->mColor),
         sf::Vertex(sf::Vector2f(p3.x, p3.y) * PPM, light->mColor)
      };

      target.draw(tri, 3, sf::Triangles);
   }
}


//-----------------------------------------------------------------------------
void RaycastLight::drawQuads(sf::RenderTarget &target, std::shared_ptr<RaycastLight::LightInstance> light) const
{
   auto lightPos = light->mPosMeters;
   const sf::Color black = {0, 0, 0, 255};

   // do not draw lights that are too far away
   auto playerBody = Player::getPlayer(0)->getBody();
   auto distanceToPlayer = (playerBody->GetWorldCenter() - light->mPosMeters).LengthSquared();

   if (distanceToPlayer > 40.0f)
   {
      return;
   }

   for (b2Body* b = Level::getCurrentLevel()->getWorld()->GetBodyList(); b; b = b->GetNext())
   {
      if (b == playerBody)
         continue;

      for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
      {
         auto shape = f->GetShape();

         auto polygonShape = dynamic_cast<b2PolygonShape*>(shape);
         auto chainShape = dynamic_cast<b2ChainShape*>(shape);

         // for now it is assumed that chainshapes are static objects only.
         // therefore no transform is applied to chainshape based objects.
         if (chainShape)
         {
            for (auto vCurr = 0; vCurr < chainShape->m_count; vCurr++)
            {
               auto vNext = vCurr + 1;
               if (vNext == chainShape->m_count)
               {
                  vNext = 0;
               }

               auto v0 = chainShape->m_vertices[vCurr];

               // printf("%f\n", (lightPos - v0).LengthSquared());

               if ((lightPos - v0).LengthSquared() > 100.0f)
                  continue;

               auto v1 = chainShape->m_vertices[vNext];
               auto v0far = 10000.0f * (v0 - lightPos);
               auto v1far = 10000.0f * (v1 - lightPos);

               sf::Vertex quad[] =
               {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, black)
               };

               target.draw(quad, 4, sf::Quads);
            }
         }
         else if (polygonShape)
         {
           for (auto vCurr = 0; vCurr < polygonShape->GetVertexCount(); vCurr++)
           {
              auto vNext = vCurr + 1;
              if (vNext == polygonShape->GetVertexCount())
              {
                 vNext = 0;
              }

              auto v0 = polygonShape->GetVertex(vCurr) + b->GetTransform().p;

              // printf("%f\n", (lightPos - v0).LengthSquared());

              if ((lightPos - v0).LengthSquared() > 100.0f)
                 continue;

              auto v1 = polygonShape->GetVertex(vNext) + b->GetTransform().p;
              auto v0far = 10000.0f * (v0 - lightPos);
              auto v1far = 10000.0f * (v1 - lightPos);

              sf::Vertex quad[] =
              {
                 sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, black),
                 sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, black),
                 sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, black),
                 sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, black)
              };

              target.draw(quad, 4, sf::Quads);
           }
         }
      }
   }
}


//-----------------------------------------------------------------------------
void RaycastLight::onDraw1(sf::RenderTarget& target, sf::RenderStates /*states*/) const
{
   // https://open.gl/depthstencils
   // https://learnopengl.com/#!Advanced-OpenGL/Stencil-testing

   // states.shader = &mShader;

   for (auto light : mLights)
   {
      // fill stencil buffer
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

      drawTriangles(target, light);

      // draw light quads with stencil boundaries
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 1, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      sf::RenderStates lightRs{sf::BlendAdd};
      target.draw(light->mSprite, lightRs);
   }

   glDisable(GL_STENCIL_TEST);

   /*
   // fill stencil buffer
   glClear(GL_STENCIL_BUFFER_BIT);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS, 1, 1);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

   for (auto light : mLights)
   {
      drawTriangles(target, light);
   }

   // draw light quads with stencil boundaries
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glStencilFunc(GL_EQUAL, 1, 1);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

   sf::RenderStates lightRs{sf::BlendAdd};
   for (auto light : mLights)
   {
      target.draw(light->mSprite, lightRs);
   }

   glDisable(GL_STENCIL_TEST);
   */
}


//-----------------------------------------------------------------------------
void RaycastLight::onDraw(sf::RenderTarget &target, sf::RenderStates /*states*/) const
{
   for (auto light : mLights)
   {
      // fill stencil buffer
      glClear(GL_STENCIL_BUFFER_BIT);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

      drawQuads(target, light);

      // draw light quads with stencil boundaries
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 0, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      sf::RenderStates lightRs{sf::BlendAdd};
      target.draw(light->mSprite, lightRs);
   }

   glDisable(GL_STENCIL_TEST);
}


//-----------------------------------------------------------------------------
void RaycastLight::debug() const
{
   for (auto light : mLights)
   {
      auto p1 = light->mPosMeters;

      for (auto p2 : light->mIntersections)
      {
         sf::Vertex line[] =
         {
            sf::Vertex(sf::Vector2f(p1.x, p1.y) * PPM, light->mColor),
            sf::Vertex(sf::Vector2f(p2.x, p2.y) * PPM, light->mColor)
         };

         printf("ray: %f, %f -> %f, %f\n", p1.x, p1.y, p2.x, p2.y);
      }
   }

   exit(0);
}

