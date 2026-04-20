#include "lightsystem.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/level/fixturenode.h"
#include "game/level/level.h"
#include "game/player/player.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>

// #define DEBUG_DRAW_LIGHT_SYSTEM

#ifdef DEBUG_DRAW_LIGHT_SYSTEM
#include "game/debug/debugdraw.h"
#endif

namespace
{
constexpr auto max_distance_m2 = 400.0f;  // depends on the view dimensions

// SFML3 overrides raw glStencilFunc/Op calls with glDisable(GL_STENCIL_TEST) on every draw.
// All stencil work must go through sf::RenderStates::stencilMode so SFML manages it correctly.

// write pass: write 1 to stencil for every fragment that should be in shadow.
// stencilOnly=true suppresses color writes (equivalent to glColorMask(false,...)).
const sf::StencilMode stencil_write_mode{
   sf::StencilComparison::Always,
   sf::StencilUpdateOperation::Replace,
   1,    // reference — writes 1 to mark occluded pixels
   ~0u,  // mask
   true  // stencilOnly: no color output
};

// read pass: only draw where stencil == 0 (not occluded by any shadow).
const sf::StencilMode stencil_test_mode{
   sf::StencilComparison::Equal,
   sf::StencilUpdateOperation::Keep,
   0,     // reference — 0 means "lit"
   ~0u,   // mask
   false  // write color
};
}  // namespace

LightSystem::LightSystem()
{
   // prepare unit circle for circle shapes
   for (auto i = 0u; i < segment_count; i++)
   {
      auto angle = (2.0 * std::numbers::pi) * (i / static_cast<double>(segment_count));

      auto x_normalized = static_cast<float>(cos(angle));
      auto y_normalized = static_cast<float>(sin(angle));

      _unit_circle[i] = b2Vec2{x_normalized, y_normalized};
   }

   if (!_light_shader.loadFromFile("data/shaders/light.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading bump mapping shader";
   }
}

void LightSystem::drawShadowQuads(
   sf::RenderTarget& target,
   std::shared_ptr<LightSystem::LightInstance> light,
   const std::vector<b2Body*>& candidates
) const
{
   const auto light_pos_m = light->_pos_m + light->_center_offset_m;

   for (auto* body : candidates)
   {
      // skip bodies that belong to the light's own mechanism
      // (e.g. rope chain elements) - they produce degenerate
      // or unwanted shadow quads relative to the light source.
      if (light->_excluded_bodies.count(body))
      {
         continue;
      }
      for (auto* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
      {
         // if something doesn't collide, it probably shouldn't have any impact on lighting, too.
         // however, the rest of the body should of course cast a shadow.
         if (fixture->IsSensor())
         {
            continue;
         }

         auto* shape = fixture->GetShape();

         auto* shape_polygon = dynamic_cast<b2PolygonShape*>(shape);
         auto* shape_chain = dynamic_cast<b2ChainShape*>(shape);
         auto* shape_circle = dynamic_cast<b2CircleShape*>(shape);

         if (shape_circle)
         {
            // do not draw lights that are too far away
            const auto center = shape_circle->m_p + body->GetTransform().p;
            if ((light_pos_m - center).LengthSquared() > max_distance_m2)
            {
               continue;
            }

            std::array<b2Vec2, segment_count> circle_positions;
            for (auto i = 0u; i < segment_count; i++)
            {
               circle_positions[i] = b2Vec2{
                  center.x + _unit_circle[i].x * shape_circle->m_radius * 1.2f, center.y + _unit_circle[i].y * shape_circle->m_radius * 1.2f
               };
            }

            for (auto pos_current = 0u; pos_current < circle_positions.size(); pos_current++)
            {
               auto pos_next = pos_current + 1;
               if (pos_next == circle_positions.size())
               {
                  pos_next = 0;
               }

               // v0      v0_far
               //  x------x
               //  |    / |
               //  |   /  |
               //  |  /   |
               //  x------x
               // v1      v1_far

               const auto& v0 = circle_positions[pos_current];
               const auto& v1 = circle_positions[pos_next];
               const auto v0far = 10000.0f * (v0 - light_pos_m);
               const auto v1far = 10000.0f * (v1 - light_pos_m);

               std::array<sf::Vertex, 6> quad = {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),

                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{stencil_write_mode});
            }
         }
         else if (shape_chain)
         {
            // for now it is assumed that chainshapes are static objects only.
            // therefore no transform is applied to chainshape based objects.
            //
            // iterate m_count - 1 edges: for CreateLoop chains m_vertices[m_count-1] == m_vertices[0]
            // so the closing edge is naturally covered; for open CreateChain shapes wrapping to 0
            // would create a phantom closing edge that does not exist.
            for (auto pos_current = 0; pos_current < shape_chain->m_count - 1; pos_current++)
            {
               auto pos_next = pos_current + 1;

               const auto& vertex_0 = shape_chain->m_vertices[pos_current];
               const auto& vertex_1 = shape_chain->m_vertices[pos_next];

               if ((light_pos_m - vertex_0).LengthSquared() > max_distance_m2 && (light_pos_m - vertex_1).LengthSquared() > max_distance_m2)
               {
                  continue;
               }

               const auto v0_far = 10000.0f * (vertex_0 - light_pos_m);
               const auto v1_far = 10000.0f * (vertex_1 - light_pos_m);

               std::array<sf::Vertex, 6> quad = {
                  sf::Vertex(sf::Vector2f(vertex_0.x, vertex_0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0_far.x, v0_far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1_far.x, v1_far.y) * PPM, sf::Color::Black),

                  sf::Vertex(sf::Vector2f(vertex_0.x, vertex_0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1_far.x, v1_far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(vertex_1.x, vertex_1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{stencil_write_mode});
            }
         }
         else if (shape_polygon)
         {
            // use m_count (vertex count), NOT GetChildCount() which always returns 1 for polygons.
            for (auto pos_current = 0; pos_current < shape_polygon->m_count; pos_current++)
            {
               auto pos_next = pos_current + 1;
               if (pos_next == shape_polygon->m_count)
               {
                  pos_next = 0;
               }

               const auto v0 = shape_polygon->m_vertices[pos_current] + body->GetTransform().p;

               if ((light_pos_m - v0).LengthSquared() > max_distance_m2)
               {
                  continue;
               }

               const auto v1 = shape_polygon->m_vertices[pos_next] + body->GetTransform().p;
               const auto v0far = 10000.0f * (v0 - light_pos_m);
               const auto v1far = 10000.0f * (v1 - light_pos_m);

               std::array<sf::Vertex, 6> quad = {
                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v0far.x, v0far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),

                  sf::Vertex(sf::Vector2f(v0.x, v0.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1far.x, v1far.y) * PPM, sf::Color::Black),
                  sf::Vertex(sf::Vector2f(v1.x, v1.y) * PPM, sf::Color::Black)
               };

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{stencil_write_mode});
            }
         }
      }
   }
}

sf::Vector2f mapCoordsToPixelNormalized(const sf::Vector2f& point, const sf::View& view)
{
   // first, transform the point by the view matrix
   sf::Vector2f normalized = view.getTransform().transformPoint(point);

   // then convert to viewport coordinates
   sf::Vector2f pixel;

   pixel.x = (normalized.x + 1.0f) / 2.0f;
   pixel.y = (-normalized.y + 1.0f) / 2.0f;

   return pixel;
}

sf::Vector2f mapCoordsToPixelScreenDimension(sf::RenderTarget& target, const sf::Vector2f& point, const sf::View& view)
{
   // first, transform the point by the view matrix
   sf::Vector2f normalized = view.getTransform().transformPoint(point);

   // then convert to viewport coordinates
   sf::Vector2f pixel;
   const auto viewport = target.getViewport(view);
   pixel.x = ((normalized.x + 1.0f) / (2.0f * static_cast<float>(viewport.size.x))) + static_cast<float>(viewport.position.x);
   pixel.y = ((-normalized.y + 1.0f) / (2.0f * static_cast<float>(viewport.size.y))) + static_cast<float>(viewport.position.y);

   return pixel;
}

void LightSystem::drawOccluders(sf::RenderTarget& target) const
{
   if (_occluder_callback)
   {
      _occluder_callback(target);
   }
}

void LightSystem::setOccluderCallback(OccluderDrawCallback callback)
{
   _occluder_callback = std::move(callback);
}

void LightSystem::updateLightShader(sf::RenderTarget& target)
{
   int32_t light_id = 0;

   _light_shader.setUniform("u_light_count", static_cast<int32_t>(_active_lights.size()));
   _light_shader.setUniform("u_resolution", sf::Glsl::Vec2(static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y)));
   _light_shader.setUniform("u_ambient", sf::Glsl::Vec4(_ambient_color[0], _ambient_color[1], _ambient_color[2], _ambient_color[3]));

   static const std::array<std::string, 6> position_uniforms = {
      "u_lights[0]._position",
      "u_lights[1]._position",
      "u_lights[2]._position",
      "u_lights[3]._position",
      "u_lights[4]._position",
      "u_lights[5]._position"
   };
   static const std::array<std::string, 6> color_uniforms = {
      "u_lights[0]._color", "u_lights[1]._color", "u_lights[2]._color", "u_lights[3]._color", "u_lights[4]._color", "u_lights[5]._color"
   };

   for (auto& light : _active_lights)
   {
      if (light_id >= static_cast<int32_t>(position_uniforms.size()))
      {
         break;
      }

      // transform light coordinates from box2d to normalized screen coordinates
      sf::Vector2f light_screen_pos = mapCoordsToPixelNormalized(
         {light->_pos_m.x * PPM + light->_center_offset_px.x, light->_pos_m.y * PPM + light->_center_offset_px.y},
         *Level::getCurrentLevel()->getLevelView().get()
      );

      _light_shader.setUniform(
         position_uniforms[light_id],
         sf::Glsl::Vec3(
            static_cast<float>(light_screen_pos.x),
            static_cast<float>(1.0f - light_screen_pos.y),
            0.075f  // default z
         )
      );

      _light_shader.setUniform(
         color_uniforms[light_id],
         sf::Glsl::Vec4(
            static_cast<float>(light->_color.r) / 255.0f,
            static_cast<float>(light->_color.g) / 255.0f,
            static_cast<float>(light->_color.b) / 255.0f,
            static_cast<float>(light->_color.a) / 255.0f
         )
      );

      // Log::Info()
      //    << "light position on screen "
      //    << id << ": "
      //    << light_screen_pos.x << ", "
      //    << light_screen_pos.y << " | "
      //    << "render target size is: "
      //    << target.getSize().x << ", "
      //    << target.getSize().y;

      light_id++;
   }
}

void LightSystem::draw(sf::RenderTarget& target1, sf::RenderTarget& target2, sf::RenderStates /*states*/)
{
   _active_lights.clear();

   auto* player_body = Player::getCurrent()->getBody();

   for (const auto& light : _lights)
   {
      if (!light->_enabled)
      {
         continue;
      }

      // don't draw lights that are too far away
      auto distanceToPlayer = (player_body->GetWorldCenter() - light->_pos_m).LengthSquared();
      if (distanceToPlayer > max_distance_m2)
      {
         continue;
      }

      _active_lights.push_back(light);
   }

   // sort by distance so the closest lights always take the available channels
   std::ranges::sort(
      _active_lights,
      [&player_body](const auto& a, const auto& b)
      { return (player_body->GetWorldCenter() - a->_pos_m).LengthSquared() < (player_body->GetWorldCenter() - b->_pos_m).LengthSquared(); }
   );

   constexpr auto max_lights = 6;
   if (_active_lights.size() > max_lights)
   {
      static auto warned = false;
      if (!warned)
      {
         Log::Warning() << "LightSystem: " << _active_lights.size() << " active lights, only " << max_lights << " can be drawn";
         warned = true;
      }
      _active_lights.resize(max_lights);
   }

   // pre-build shadow caster candidates once per frame — player, disabled bodies, and
   // enemies are excluded here so drawShadowQuads only needs to check per-light exclusions.
   std::vector<b2Body*> shadow_candidates;
   const auto& world = Level::getCurrentLevel()->getWorld();
   for (auto* body = world->GetBodyList(); body; body = body->GetNext())
   {
      if (body == player_body || !body->IsEnabled())
      {
         continue;
      }
      bool skip = false;
      for (auto* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
      {
         if (fixture->GetFilterData().categoryBits & CategoryNoCastShadow)
         {
            skip = true;
            break;
         }
         auto* user_data = fixture->GetUserData().pointer;
         if (user_data && static_cast<FixtureNode*>(user_data)->getType() == ObjectType::ObjectTypeEnemy)
         {
            skip = true;
            break;
         }
      }
      if (!skip)
      {
         shadow_candidates.push_back(body);
      }
   }

   // draw sprites to channels (lights 0-2 to target1 RGB, lights 3-5 to target2 RGB)
   // we skip alpha channels because they're harder to work with
   int32_t channel_index = 0;
   for (const auto& light : _active_lights)
   {
      if (channel_index >= 6)
      {
         break;  // max 6 lights (2 textures × RGB, skip alpha)
      }

      // determine which target and which RGB channel
      sf::RenderTarget& target = (channel_index < 3) ? target1 : target2;
      int local_channel = channel_index % 3;

      // clear the stencil buffer for this light via SFML's API.
      // raw glClear(GL_STENCIL_BUFFER_BIT) would be fine here but using the SFML
      // path keeps all stencil management consistent and avoids context surprises.
      target.clearStencil(0);

      // draw occluders and shadow quads into the stencil buffer only.
      // each draw call carries stencil_write_mode so SFML enables the stencil test
      // and writes 1 for every occluded pixel instead of disabling it (default behaviour).
      drawOccluders(target);
      drawShadowQuads(target, light, shadow_candidates);

      // draw the light sprite only where stencil == 0 (not occluded).
      sf::Color channel_color;
      if (local_channel == 0)
      {
         channel_color = sf::Color(255, 0, 0, 255);  // red
      }
      else if (local_channel == 1)
      {
         channel_color = sf::Color(0, 255, 0, 255);  // green
      }
      else
      {
         channel_color = sf::Color(0, 0, 255, 255);  // blue
      }

      light->_sprite->setColor(channel_color);

      sf::RenderStates render_states{sf::BlendAdd};
      render_states.stencilMode = stencil_test_mode;
      target.draw(*light->_sprite, render_states);

      channel_index++;
   }

   // Log::Info() << _active_lights.size() << " active light sources";
}

void LightSystem::draw(
   sf::RenderTarget& target,
   const std::shared_ptr<sf::RenderTexture>& color_map,
   const std::shared_ptr<sf::RenderTexture>& light_map,
   const std::shared_ptr<sf::RenderTexture>& light_map2,
   const std::shared_ptr<sf::RenderTexture>& normal_map
)
{
   // texture uniforms only need to be rebound when the render texture objects change
   // (i.e. on resize); skip the setUniform calls when the pointers are unchanged.
   const auto* color_tex = &color_map->getTexture();
   const auto* light_tex = &light_map->getTexture();
   const auto* light2_tex = &light_map2->getTexture();
   const auto* normal_tex = &normal_map->getTexture();
   if (color_tex != _last_color_map || light_tex != _last_light_map || light2_tex != _last_light_map2 || normal_tex != _last_normal_map)
   {
      _light_shader.setUniform("color_map", *color_tex);
      _light_shader.setUniform("light_map_1", *light_tex);
      _light_shader.setUniform("light_map_2", *light2_tex);
      _light_shader.setUniform("normal_map", *normal_tex);
      _last_color_map = color_tex;
      _last_light_map = light_tex;
      _last_light_map2 = light2_tex;
      _last_normal_map = normal_tex;
   }

   // update shader uniforms
   updateLightShader(target);

   sf::Sprite sprite(color_map->getTexture());
   target.draw(sprite, &_light_shader);
}

void LightSystem::drawDebug(sf::RenderTarget& target)
{
#ifdef DEBUG_DRAW_LIGHT_SYSTEM
   // debug draw light system:
   //
   // red circle = light's base position (_pos_m)
   // green circle = center position
   for (const auto& light : _lights)
   {
      // debug draw light texture quad boundaries
      const auto sprite_bounds = light->_sprite->getGlobalBounds();
      DebugDraw::drawRect(target, sprite_bounds, sf::Color::Cyan, sf::Color::Transparent);

      // debug draw light base position (without center offset)
      const auto base_pos = light->_pos_m;
      DebugDraw::drawPoint(target, base_pos, b2Color(1.0f, 0.0f, 0.0f));  // red point
      DebugDraw::drawCircle(target, base_pos, 0.1f, b2Color(1.0f, 0.0f, 0.0f));

      // debug draw light center position (with center offset)
      const auto center_pos = light->_pos_m + light->_center_offset_m;
      DebugDraw::drawPoint(target, center_pos, b2Color(0.0f, 1.0f, 0.0f));  // green point
      DebugDraw::drawCircle(target, center_pos, 0.15f, b2Color(0.0f, 1.0f, 0.0f));

      // debug draw line from base position to center position if there's an offset
      if (light->_center_offset_m.x != 0.0f || light->_center_offset_m.y != 0.0f)
      {
         DebugDraw::drawLine(target, base_pos, center_pos, b2Color(1.0f, 1.0f, 0.0f));  // yellow line
      }

      // debug draw cross at sprite center
      const auto sprite_center_px =
         sf::Vector2f(sprite_bounds.position.x + sprite_bounds.size.x * 0.5f, sprite_bounds.position.y + sprite_bounds.size.y * 0.5f);
      DebugDraw::drawPoint(target, sprite_center_px, b2Color(0.0f, 0.0f, 1.0f));  // blue point
   }
#endif
}

void LightSystem::LightInstance::deserialize(const nlohmann::json& node)
{
   if (!node.is_object())
   {
      return;
   }

   if (const auto it = node.find("color_r"); it != node.end())
   {
      _color.r = static_cast<uint8_t>(it->get<int32_t>());
   }

   if (const auto it = node.find("color_g"); it != node.end())
   {
      _color.g = static_cast<uint8_t>(it->get<int32_t>());
   }

   if (const auto it = node.find("color_b"); it != node.end())
   {
      _color.b = static_cast<uint8_t>(it->get<int32_t>());
   }

   if (const auto it = node.find("color_a"); it != node.end())
   {
      _color.a = static_cast<uint8_t>(it->get<int32_t>());
   }

   if (const auto it = node.find("width_px"); it != node.end())
   {
      _width_px = it->get<int32_t>();
   }

   if (const auto it = node.find("height_px"); it != node.end())
   {
      _height_px = it->get<int32_t>();
   }

   if (const auto it = node.find("center_offset_x_px"); it != node.end())
   {
      _center_offset_px.x = it->get<int32_t>();
      _center_offset_m.x = _center_offset_px.x * MPP;
   }

   if (const auto it = node.find("center_offset_y_px"); it != node.end())
   {
      _center_offset_px.y = it->get<int32_t>();
      _center_offset_m.y = _center_offset_px.y * MPP;
   }

   if (const auto it = node.find("texture"); it != node.end())
   {
      _texture = TexturePool::getInstance().get(it->get<std::string>());
   }

   if (const auto it = node.find("enabled"); it != node.end())
   {
      _enabled = it->get<bool>();
   }

   if (_sprite && _texture)
   {
      _sprite->setTexture(*_texture);
      _sprite->setTextureRect(
         sf::IntRect({0, 0}, {static_cast<int32_t>(_texture->getSize().x), static_cast<int32_t>(_texture->getSize().y)})
      );
      _sprite->setScale({static_cast<float>(_width_px) / _texture->getSize().x, static_cast<float>(_height_px) / _texture->getSize().y});
      _sprite->setColor(_color);
   }
}

void LightSystem::LightInstance::updateSpritePosition() const
{
   _sprite->setPosition(sf::Vector2f(_pos_m.x * PPM - _width_px * 0.5f, _pos_m.y * PPM - _height_px * 0.5f));
}

std::shared_ptr<LightSystem::LightInstance> LightSystem::createLightInstance(GameNode* parent, const GameDeserializeData& data)
{
   auto light = std::make_shared<LightSystem::LightInstance>(parent);

   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";

   if (data._tmx_object && data._tmx_object->_properties)
   {
      auto it = data._tmx_object->_properties->_map.find("color");
      if (it != data._tmx_object->_properties->_map.end())
      {
         rgba = TmxTools::color(it->second->_value_string.value());
      }

      it = data._tmx_object->_properties->_map.find("texture");
      if (it != data._tmx_object->_properties->_map.end())
      {
         texture = (std::filesystem::path("data/light/") / it->second->_value_string.value()).string();
      }
      else
      {
         Log::Warning() << "no texture property found, using default: " << texture;
      }

      // A) center of the physical light is in the center of the textured quad
      //
      //   +----+----+
      //   |    |    |
      //   |   \|/   |
      //   +----+----+
      //   |   /|\   |
      //   |    |    |
      //   +----+----+
      //
      // B) center of the phyisical light is not in the center of the textured quad
      //    but has some offset to it. here the center is, say -24px, higher
      //
      //   +----+----+
      //   |   \|/   |
      //   +----+----+ center_offset_x_px = 0
      //   |   /|\   | center_offset_y_px= -24
      //   |    |    |
      //   |    |    |
      //   +----+----+

      // read offset
      it = data._tmx_object->_properties->_map.find("center_offset_x_px");
      if (it != data._tmx_object->_properties->_map.end())
      {
         light->_center_offset_px.x = it->second->_value_int.value();
         light->_center_offset_m.x = it->second->_value_int.value() * MPP;
      }

      it = data._tmx_object->_properties->_map.find("center_offset_y_px");
      if (it != data._tmx_object->_properties->_map.end())
      {
         light->_center_offset_px.y = it->second->_value_int.value();
         light->_center_offset_m.y = it->second->_value_int.value() * MPP;
      }

      // read falloff
      //
      // constant falloff:   basically ambient light amount
      // linear falloff:    light = 1 / distance to light
      // quadratic falloff: light = 1 / (distance to light)^2
      //
      //                                                                1.0
      // attenuation = --------------------------------------------------------------------------------------------
      //                (constant falloff + (linear falloff * distance) + (quadratic falloff * distane * distance))
      //
      it = data._tmx_object->_properties->_map.find("falloff_constant");
      if (it != data._tmx_object->_properties->_map.end())
      {
         light->_falloff[0] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("falloff_linear");
      if (it != data._tmx_object->_properties->_map.end())
      {
         light->_falloff[1] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("falloff_quadratic");
      if (it != data._tmx_object->_properties->_map.end())
      {
         light->_falloff[2] = it->second->_value_float.value();
      }
   }

   if (data._tmx_object)
   {
      light->_width_px = static_cast<int32_t>(data._tmx_object->_width_px);
      light->_height_px = static_cast<int32_t>(data._tmx_object->_height_px);

      // set up the box2d position of the light
      light->_pos_m = b2Vec2(
         (data._tmx_object->_x_px * MPP) + (data._tmx_object->_width_px * 0.5f * MPP),
         (data._tmx_object->_y_px * MPP) + (data._tmx_object->_height_px * 0.5f * MPP)
      );

      light->setObjectId(data._tmx_object->_name);
   }

   light->_color.r = rgba[0];
   light->_color.g = rgba[1];
   light->_color.b = rgba[2];
   light->_color.a = rgba[3];

   // for now the sprite color is left white since it'll be used as attenuation value in the light shader
   //
   // light->_sprite.setColor(light->_color);

   light->_texture = TexturePool::getInstance().get(texture);
   light->_sprite = std::make_unique<sf::Sprite>(*light->_texture);
   light->_sprite->setTextureRect(
      sf::IntRect({0, 0}, {static_cast<int32_t>(light->_texture->getSize().x), static_cast<int32_t>(light->_texture->getSize().y)})
   );

   light->updateSpritePosition();

   const auto scale_x = static_cast<float>(light->_width_px) / static_cast<float>(light->_texture->getSize().x);
   const auto scale_y = static_cast<float>(light->_height_px) / static_cast<float>(light->_texture->getSize().y);
   light->_sprite->setScale({scale_x, scale_y});

   return light;
}

std::shared_ptr<LightSystem::LightInstance> LightSystem::createLightInstance(GameNode* parent, const nlohmann::json& node)
{
   auto light = std::make_shared<LightSystem::LightInstance>(parent);
   light->_texture = TexturePool::getInstance().get("data/light/smooth.png");
   light->_sprite = std::make_unique<sf::Sprite>(*light->_texture);
   light->_sprite->setTextureRect(
      sf::IntRect({0, 0}, {static_cast<int32_t>(light->_texture->getSize().x), static_cast<int32_t>(light->_texture->getSize().y)})
   );
   light->deserialize(node);
   return light;
}

void LightSystem::increaseAmbient(float amount)
{
   std::ranges::transform(
      _ambient_color.begin(), _ambient_color.end(), _ambient_color.begin(), [amount](auto value) { return value + amount; }
   );
}

void LightSystem::decreaseAmbient(float amount)
{
   std::ranges::transform(
      _ambient_color.begin(), _ambient_color.end(), _ambient_color.begin(), [amount](auto value) { return value - amount; }
   );
}

void LightSystem::setAmbient(sf::Color color)
{
   _ambient_color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
}
