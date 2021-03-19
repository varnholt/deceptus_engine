#include "lightsystem.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/debugdraw.h"
#include "game/level.h"
#include "game/player/player.h"
#include "texturepool.h"

#include <iostream>
#include <math.h>

#include <SFML/OpenGL.hpp>



//-----------------------------------------------------------------------------
namespace
{
static constexpr auto segments = 20;
static constexpr auto max_distance_m2 = 100.0f; // depends on the view dimensions
const sf::Color black{0, 0, 0, 255};
std::array<b2Vec2, segments> unit_circle;
}


//-----------------------------------------------------------------------------
LightSystem::LightSystem()
{
   // prepare unit circle for circle shapes
   for (auto i = 0u; i < segments; i++)
   {
      auto angle = (2.0 * M_PI) * (i / static_cast<double>(segments));

      auto x = static_cast<float>(cos(angle));
      auto y = static_cast<float>(sin(angle));

      unit_circle[i] = b2Vec2{x, y};
   }

   if (!_light_shader.loadFromFile("data/shaders/light.frag", sf::Shader::Fragment))
   {
      std::cout << "[!] error loading bump mapping shader" << std::endl;
   }
}


//-----------------------------------------------------------------------------
void LightSystem::drawShadowQuads(sf::RenderTarget& target, std::shared_ptr<LightSystem::LightInstance> light) const
{
   // do not draw lights that are too far away
   auto player_body = Player::getCurrent()->getBody();

   auto light_pos_m = light->_pos_m;

   for (b2Body* b = Level::getCurrentLevel()->getWorld()->GetBodyList(); b; b = b->GetNext())
   {
      if (b == player_body)
         continue;

      if (!b->IsActive())
      {
         continue;
      }

      for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
      {
         auto shape = f->GetShape();

         auto shape_polygon = dynamic_cast<b2PolygonShape*>(shape);
         auto shape_chain = dynamic_cast<b2ChainShape*>(shape);
         auto shape_circle = dynamic_cast<b2CircleShape*>(shape);

         if (shape_circle)
         {
            auto center = shape_circle->GetVertex(0) + b->GetTransform().p;
            if ((light_pos_m - center).LengthSquared() > max_distance_m2)
               continue;

            std::array<b2Vec2, segments> circle_positions;
            for (auto i = 0u; i < segments; i++)
            {
               circle_positions[i] = b2Vec2{
                  center.x + unit_circle[i].x * shape_circle->m_radius * 1.2f,
                  center.y + unit_circle[i].y * shape_circle->m_radius * 1.2f
               };
            }

            for (auto pos_current = 0u; pos_current < circle_positions.size(); pos_current++)
            {
               auto pos_next = pos_current + 1;
               if (pos_next == circle_positions.size())
               {
                  pos_next = 0;
               }

               auto v0 = circle_positions[pos_current];
               auto v1 = circle_positions[pos_next];

               auto v0far = 10000.0f * (v0 - light_pos_m);
               auto v1far = 10000.0f * (v1 - light_pos_m);

               sf::Vertex quad[] =
               {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad, 4, sf::Quads);
            }
         }
         else if (shape_chain)
         {
            // for now it is assumed that chainshapes are static objects only.
            // therefore no transform is applied to chainshape based objects.

            for (auto pos_current = 0; pos_current < shape_chain->m_count; pos_current++)
            {
               auto pos_next = pos_current + 1;
               if (pos_next == shape_chain->m_count)
               {
                  pos_next = 0;
               }

               auto v0 = shape_chain->m_vertices[pos_current];
               auto v1 = shape_chain->m_vertices[pos_next];

               // printf("%f\n", (lightPos - v0).LengthSquared());

               if (
                     (light_pos_m - v0).LengthSquared() > max_distance_m2
                  && (light_pos_m - v1).LengthSquared() > max_distance_m2
               )
               {
                  continue;
               }

               auto v0far = 10000.0f * (v0 - light_pos_m);
               auto v1far = 10000.0f * (v1 - light_pos_m);

               sf::Vertex quad[] =
               {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad, 4, sf::Quads);
            }
         }
         else if (shape_polygon)
         {
            for (auto pos_current = 0; pos_current < shape_polygon->GetVertexCount(); pos_current++)
            {
               auto pos_next = pos_current + 1;
               if (pos_next == shape_polygon->GetVertexCount())
               {
                  pos_next = 0;
               }

               auto v0 = shape_polygon->GetVertex(pos_current) + b->GetTransform().p;

               // printf("%f\n", (lightPos - v0).LengthSquared());

               if ((light_pos_m - v0).LengthSquared() > max_distance_m2)
                  continue;

               auto v1 = shape_polygon->GetVertex(pos_next) + b->GetTransform().p;
               auto v0far = 10000.0f * (v0 - light_pos_m);
               auto v1far = 10000.0f * (v1 - light_pos_m);

               sf::Vertex quad[] =
               {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad, 4, sf::Quads);
            }
         }
      }
   }
}


//-----------------------------------------------------------------------------
void LightSystem::draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const
{
   auto player_body = Player::getCurrent()->getBody();

   for (const auto& light : _lights)
   {
      // don't draw lights that are too far away
      auto distanceToPlayer = (player_body->GetWorldCenter() - light->_pos_m).LengthSquared();

      if (distanceToPlayer > max_distance_m2)
      {
         continue;
      }

      // fill stencil buffer
      glClear(GL_STENCIL_BUFFER_BIT);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

      drawShadowQuads(target, light);

      // draw light quads with stencil boundaries
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 0, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      sf::RenderStates lightRs{sf::BlendAdd};
      target.draw(light->_sprite, lightRs);
   }

   glDisable(GL_STENCIL_TEST);
}


//-----------------------------------------------------------------------------
void LightSystem::draw(
   sf::RenderTarget& target,
   const std::shared_ptr<sf::RenderTexture>& color_map,
   const std::shared_ptr<sf::RenderTexture>& light_map,
   const std::shared_ptr<sf::RenderTexture>& normal_map
)
{
   _light_shader.setUniform("color_map", color_map->getTexture());
   _light_shader.setUniform("light_map", light_map->getTexture());
   _light_shader.setUniform("normal_map", normal_map->getTexture());

   sf::Sprite sprite;
   sprite.setTexture(color_map->getTexture());
   target.draw(sprite, &_light_shader);
}



//-----------------------------------------------------------------------------
void LightSystem::LightInstance::updateSpritePosition()
{
   _sprite.setPosition(
      sf::Vector2f(
         _pos_m.x * PPM - _width_px * 0.5f + _offset_x_px,
         _pos_m.y * PPM - _height_px * 0.5f + _offset_y_px
      )
   );
}



//-----------------------------------------------------------------------------
std::shared_ptr<LightSystem::LightInstance> LightSystem::createLightInstance(TmxObject* tmx_object)
{
   auto light = std::make_shared<LightSystem::LightInstance>();

   if (tmx_object)
   {
      light->_width_px = static_cast<int>(tmx_object->mWidth);
      light->_height_px = static_cast<int>(tmx_object->mHeight);

      light->_pos_m = b2Vec2(
        tmx_object->mX * MPP + (tmx_object->mWidth * 0.5f) * MPP,
        tmx_object->mY * MPP + (tmx_object->mHeight * 0.5f) * MPP
      );
   }

   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";

   if (tmx_object && tmx_object->mProperties != nullptr)
   {
      auto it = tmx_object->mProperties->mMap.find("color");
      if (it != tmx_object->mProperties->mMap.end())
      {
         rgba = TmxTools::color(it->second->mValueStr.value());
      }

      it = tmx_object->mProperties->mMap.find("texture");
      if (it != tmx_object->mProperties->mMap.end())
      {
         texture = (std::filesystem::path("data/light/") / it->second->mValueStr.value()).string();
      }
   }

   light->_color.r = rgba[0];
   light->_color.g = rgba[1];
   light->_color.b = rgba[2];
   light->_color.a = rgba[3];
   light->_sprite.setColor(light->_color);

   light->_texture = TexturePool::getInstance().get(texture);
   light->_sprite.setTexture(*light->_texture);
   light->_sprite.setTextureRect(
     sf::IntRect(
       0,
       0,
       static_cast<int32_t>(light->_texture->getSize().x),
       static_cast<int32_t>(light->_texture->getSize().y)
     )
   );

   light->updateSpritePosition();

   auto scale = static_cast<float>(light->_width_px) / light->_texture->getSize().x;
   light->_sprite.setScale(scale, scale);

   return light;
}

