#include "raycastlight.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/debugdraw.h"
#include "game/level.h"
#include "game/player/player.h"
#include "texturepool.h"

#include <iostream>

#include <SFML/OpenGL.hpp>

#include <math.h>


//-----------------------------------------------------------------------------
namespace
{
   static const auto segments = 20;
   static const sf::Color black = {0, 0, 0, 255};
   static std::array<b2Vec2, segments> unitCircle;
   static const auto maxDistance = 100.0f; // depends on the view dimensions
}


//-----------------------------------------------------------------------------
RaycastLight::RaycastLight()
 : Effect("raycast light")
{
   // prepare unit circle for circle shapes
   for (auto i = 0u; i < segments; i++)
   {
      auto angle = (2.0 * M_PI) * (i / static_cast<double>(segments));

      auto x = static_cast<float>(cos(angle));
      auto y = static_cast<float>(sin(angle));

      unitCircle[i] = b2Vec2{x, y};
   }
}


//-----------------------------------------------------------------------------
bool RaycastLight::onLoad()
{
   return true;
}


//-----------------------------------------------------------------------------
void RaycastLight::onUpdate(const sf::Time& /*time*/, float /*x*/, float /*y*/)
{
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
void RaycastLight::drawQuads(sf::RenderTarget& target, std::shared_ptr<RaycastLight::LightInstance> light) const
{
   // do not draw lights that are too far away
   auto playerBody = Player::getCurrent()->getBody();

   auto lightPos = light->mPosMeters;

   for (b2Body* b = Level::getCurrentLevel()->getWorld()->GetBodyList(); b; b = b->GetNext())
   {
      if (b == playerBody)
         continue;

      if (!b->IsActive())
      {
         continue;
      }

      for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
      {
         auto shape = f->GetShape();

         auto polygonShape = dynamic_cast<b2PolygonShape*>(shape);
         auto chainShape = dynamic_cast<b2ChainShape*>(shape);
         auto circleShape = dynamic_cast<b2CircleShape*>(shape);

         if (circleShape)
         {
            auto center = circleShape->GetVertex(0) + b->GetTransform().p;
            if ((lightPos - center).LengthSquared() > maxDistance)
               continue;

            std::array<b2Vec2, segments> circlePositions;
            for (auto i = 0u; i < segments; i++)
            {
               circlePositions[i] = b2Vec2{
                  center.x + unitCircle[i].x * circleShape->m_radius * 1.2f,
                  center.y + unitCircle[i].y * circleShape->m_radius * 1.2f
               };
            }

            for (auto vCurr = 0u; vCurr < circlePositions.size(); vCurr++)
            {
               auto vNext = vCurr + 1;
               if (vNext == circlePositions.size())
               {
                  vNext = 0;
               }

               auto v0 = circlePositions[vCurr];
               auto v1 = circlePositions[vNext];

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
         else if (chainShape)
         {
            // for now it is assumed that chainshapes are static objects only.
            // therefore no transform is applied to chainshape based objects.

            for (auto vCurr = 0; vCurr < chainShape->m_count; vCurr++)
            {
               auto vNext = vCurr + 1;
               if (vNext == chainShape->m_count)
               {
                  vNext = 0;
               }

               auto v0 = chainShape->m_vertices[vCurr];
               auto v1 = chainShape->m_vertices[vNext];

               // printf("%f\n", (lightPos - v0).LengthSquared());

               if (
                     (lightPos - v0).LengthSquared() > maxDistance
                  && (lightPos - v1).LengthSquared() > maxDistance
               )
               {
                  continue;
               }

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

               if ((lightPos - v0).LengthSquared() > maxDistance)
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
void RaycastLight::onDraw(sf::RenderTarget& target, sf::RenderStates /*states*/) const
{
   auto playerBody = Player::getCurrent()->getBody();

   for (const auto& light : mLights)
   {
       // don't draw lights that are too far away
       auto distanceToPlayer = (playerBody->GetWorldCenter() - light->mPosMeters).LengthSquared();

       if (distanceToPlayer > maxDistance)
       {
          continue;
       }

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
         // sf::Vertex line[] =
         // {
         //    sf::Vertex(sf::Vector2f(p1.x, p1.y) * PPM, light->mColor),
         //    sf::Vertex(sf::Vector2f(p2.x, p2.y) * PPM, light->mColor)
         // };

         printf(
            "ray: %f, %f -> %f, %f\n",
            static_cast<double>(p1.x),
            static_cast<double>(p1.y),
            static_cast<double>(p2.x),
            static_cast<double>(p2.y)
         );
      }
   }
}


//-----------------------------------------------------------------------------
void RaycastLight::LightInstance::updateSpritePosition()
{
   mSprite.setPosition(
      sf::Vector2f(
         mPosMeters.x * PPM - mWidth * 0.5f + mOffsetX,
         mPosMeters.y * PPM - mHeight * 0.5f + mOffsetY
      )
   );
}



//-----------------------------------------------------------------------------
std::shared_ptr<RaycastLight::LightInstance> RaycastLight::deserialize(TmxObject* tmxObject)
{
   auto light = std::make_shared<RaycastLight::LightInstance>();

   if (tmxObject)
   {
      light->mWidth = static_cast<int>(tmxObject->mWidth);
      light->mHeight = static_cast<int>(tmxObject->mHeight);

      light->mPosMeters = b2Vec2(
        tmxObject->mX * MPP + (tmxObject->mWidth * 0.5f) * MPP,
        tmxObject->mY * MPP + (tmxObject->mHeight * 0.5f) * MPP
      );
   }

   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";

   if (tmxObject && tmxObject->mProperties != nullptr)
   {
      auto it = tmxObject->mProperties->mMap.find("color");
      if (it != tmxObject->mProperties->mMap.end())
      {
         rgba = TmxTools::color(it->second->mValueStr.value());
      }

      it = tmxObject->mProperties->mMap.find("texture");
      if (it != tmxObject->mProperties->mMap.end())
      {
         texture = (std::filesystem::path("data/light/") / it->second->mValueStr.value()).string();
      }
   }

   light->mColor.r = rgba[0];
   light->mColor.g = rgba[1];
   light->mColor.b = rgba[2];
   light->mColor.a = rgba[3];
   light->mSprite.setColor(light->mColor);

   light->mTexture = TexturePool::getInstance().get(texture);
   light->mSprite.setTexture(*light->mTexture);
   light->mSprite.setTextureRect(
     sf::IntRect(
       0,
       0,
       static_cast<int32_t>(light->mTexture->getSize().x),
       static_cast<int32_t>(light->mTexture->getSize().y)
     )
   );

   light->updateSpritePosition();

   auto scale = static_cast<float>(light->mWidth) / light->mTexture->getSize().x;
   light->mSprite.setScale(scale, scale);

   return light;
}

