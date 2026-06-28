#include "lightsystem.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/level/fixturenode.h"
#include "game/level/levelregistry.h"
#include "game/player/playerregistry.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>
#include <span>

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
   .stencilComparison = sf::StencilComparison::Always,
   .stencilUpdateOperation = sf::StencilUpdateOperation::Replace,
   .stencilOnly = true,
   .stencilReference = sf::StencilValue{1u},
   .stencilMask = sf::StencilValue{~0u}
};

// read pass: only draw where stencil == 0 (not occluded by any shadow).
const sf::StencilMode stencil_test_mode{
   .stencilComparison = sf::StencilComparison::Equal,
   .stencilUpdateOperation = sf::StencilUpdateOperation::Keep,
   .stencilOnly = false,
   .stencilReference = sf::StencilValue{0u},
   .stencilMask = sf::StencilValue{~0u}
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

   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/light.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading bump mapping shader";
      return;
   }
   _light_shader = std::move(*loaded);

   _ul_light_count  = _light_shader->getUniformLocation("u_light_count");
   _ul_resolution   = _light_shader->getUniformLocation("u_resolution");
   _ul_ambient      = _light_shader->getUniformLocation("u_ambient");
   _ul_color_map    = _light_shader->getUniformLocation("color_map");
   _ul_light_map_1  = _light_shader->getUniformLocation("light_map_1");
   _ul_light_map_2  = _light_shader->getUniformLocation("light_map_2");
   _ul_normal_map   = _light_shader->getUniformLocation("normal_map");

   static const std::array<std::string, 6> position_names = {
      "u_lights[0]._position", "u_lights[1]._position", "u_lights[2]._position",
      "u_lights[3]._position", "u_lights[4]._position", "u_lights[5]._position"
   };
   static const std::array<std::string, 6> color_names = {
      "u_lights[0]._color", "u_lights[1]._color", "u_lights[2]._color",
      "u_lights[3]._color", "u_lights[4]._color", "u_lights[5]._color"
   };
   for (auto index = 0u; index < 6u; index++)
   {
      _ul_light_positions[index] = _light_shader->getUniformLocation(position_names[index]);
      _ul_light_colors[index]    = _light_shader->getUniformLocation(color_names[index]);
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

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{.stencilMode = stencil_write_mode});
            }
         }
         else if (shape_chain)
         {
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

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{.stencilMode = stencil_write_mode});
            }
         }
         else if (shape_polygon)
         {
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

               target.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, sf::RenderStates{.stencilMode = stencil_write_mode});
            }
         }
      }
   }
}

sf::Vector2f mapCoordsToPixelNormalized(const sf::Vector2f& point, const sf::View& view)
{
   sf::Vector2f normalized = view.getTransform().transformPoint(point);
   sf::Vector2f pixel;
   pixel.x = (normalized.x + 1.0f) / 2.0f;
   pixel.y = (-normalized.y + 1.0f) / 2.0f;
   return pixel;
}

sf::Vector2f mapCoordsToPixelScreenDimension(sf::RenderTarget& target, const sf::Vector2f& point, const sf::View& view)
{
   sf::Vector2f normalized = view.getTransform().transformPoint(point);
   sf::Vector2f pixel;
   const auto viewport = view.computePixelViewport(sf::Vec2f(target.getSize()));
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
   if (!_light_shader.has_value())
   {
      return;
   }

   int32_t light_id = 0;

   if (_ul_light_count.has_value())
   {
      _light_shader->setUniform(*_ul_light_count, static_cast<int32_t>(_active_lights.size()));
   }
   if (_ul_resolution.has_value())
   {
      _light_shader->setUniform(*_ul_resolution, sf::Glsl::Vec2(static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y)));
   }
   if (_ul_ambient.has_value())
   {
      _light_shader->setUniform(*_ul_ambient, sf::Glsl::Vec4(_ambient_color[0], _ambient_color[1], _ambient_color[2], _ambient_color[3]));
   }

   for (auto& light : _active_lights)
   {
      if (light_id >= 6)
      {
         break;
      }

      sf::Vector2f light_screen_pos = mapCoordsToPixelNormalized(
         {light->_pos_m.x * PPM + light->_center_offset_px.x, light->_pos_m.y * PPM + light->_center_offset_px.y},
         *LevelRegistry::getCurrent()->getLevelView().get()
      );

      if (_ul_light_positions[light_id].has_value())
      {
         _light_shader->setUniform(
            *_ul_light_positions[light_id],
            sf::Glsl::Vec3(
               static_cast<float>(light_screen_pos.x),
               static_cast<float>(1.0f - light_screen_pos.y),
               0.075f
            )
         );
      }

      if (_ul_light_colors[light_id].has_value())
      {
         _light_shader->setUniform(
            *_ul_light_colors[light_id],
            sf::Glsl::Vec4(
               static_cast<float>(light->_color.r) / 255.0f,
               static_cast<float>(light->_color.g) / 255.0f,
               static_cast<float>(light->_color.b) / 255.0f,
               static_cast<float>(light->_color.a) / 255.0f
            )
         );
      }

      light_id++;
   }
}

void LightSystem::draw(sf::RenderTarget& target1, sf::RenderTarget& target2, sf::RenderStates /*states*/)
{
   _active_lights.clear();

   auto* player_body = PlayerRegistry::getFirst()->getBody();

   for (const auto& light : _lights)
   {
      if (!light->_enabled)
      {
         continue;
      }

      auto distanceToPlayer = (player_body->GetWorldCenter() - light->_pos_m).LengthSquared();
      if (distanceToPlayer > max_distance_m2)
      {
         continue;
      }

      _active_lights.push_back(light);
   }

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

   std::vector<b2Body*> shadow_candidates;
   const auto& world = LevelRegistry::getCurrent()->getWorld();
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

   int32_t channel_index = 0;
   for (const auto& light : _active_lights)
   {
      if (channel_index >= 6)
      {
         break;
      }

      sf::RenderTarget& target = (channel_index < 3) ? target1 : target2;
      int local_channel = channel_index % 3;

      target.clearStencil(sf::StencilValue{0u});

      drawOccluders(target);
      drawShadowQuads(target, light, shadow_candidates);

      sf::Color channel_color;
      if (local_channel == 0)
      {
         channel_color = sf::Color(255, 0, 0, 255);
      }
      else if (local_channel == 1)
      {
         channel_color = sf::Color(0, 255, 0, 255);
      }
      else
      {
         channel_color = sf::Color(0, 0, 255, 255);
      }

      light->_sprite->color = channel_color;

      sf::RenderStates render_states{.blendMode = sf::BlendAdd, .stencilMode = stencil_test_mode};

      if (light->_shader && light->_texture)
      {
         auto texture_loc = light->_shader->getUniformLocation("texture");
         if (texture_loc.hasValue())
         {
            (void)light->_shader->setUniform(*texture_loc, *light->_texture);
         }
         if (light->_shader_update_callback)
         {
            light->_shader_update_callback(*light->_shader, *light, _clock.getElapsedTime().asSeconds());
         }
         render_states.shader = light->_shader.get();
      }

      target.draw(*light->_sprite, render_states);

      channel_index++;
   }
}

void LightSystem::draw(
   sf::RenderTarget& target,
   const std::shared_ptr<sf::RenderTexture>& color_map,
   const std::shared_ptr<sf::RenderTexture>& light_map,
   const std::shared_ptr<sf::RenderTexture>& light_map2,
   const std::shared_ptr<sf::RenderTexture>& normal_map
)
{
   if (!_light_shader.has_value())
   {
      return;
   }

   const auto* color_tex  = &color_map->getTexture();
   const auto* light_tex  = &light_map->getTexture();
   const auto* light2_tex = &light_map2->getTexture();
   const auto* normal_tex = &normal_map->getTexture();

   if (color_tex != _last_color_map || light_tex != _last_light_map || light2_tex != _last_light_map2 || normal_tex != _last_normal_map)
   {
      if (_ul_color_map.has_value())
      {
         (void)_light_shader->setUniform(*_ul_color_map, *color_tex);
      }
      if (_ul_light_map_1.has_value())
      {
         (void)_light_shader->setUniform(*_ul_light_map_1, *light_tex);
      }
      if (_ul_light_map_2.has_value())
      {
         (void)_light_shader->setUniform(*_ul_light_map_2, *light2_tex);
      }
      if (_ul_normal_map.has_value())
      {
         (void)_light_shader->setUniform(*_ul_normal_map, *normal_tex);
      }
      _last_color_map  = color_tex;
      _last_light_map  = light_tex;
      _last_light_map2 = light2_tex;
      _last_normal_map = normal_tex;
   }

   updateLightShader(target);

   const sf::Texture& light_color_texture = color_map->getTexture();
   target.draw(light_color_texture, sf::RenderStates{.shader = &(*_light_shader)});
}

void LightSystem::drawDebug(sf::RenderTarget& target)
{
#ifdef DEBUG_DRAW_LIGHT_SYSTEM
   for (const auto& light : _lights)
   {
      const auto sprite_bounds = light->_sprite->getGlobalBounds();
      DebugDraw::drawRect(target, sprite_bounds, sf::Color::Cyan, sf::Color::Transparent);

      const auto base_pos = light->_pos_m;
      DebugDraw::drawPoint(target, base_pos, b2Color(1.0f, 0.0f, 0.0f));
      DebugDraw::drawCircle(target, base_pos, 0.1f, b2Color(1.0f, 0.0f, 0.0f));

      const auto center_pos = light->_pos_m + light->_center_offset_m;
      DebugDraw::drawPoint(target, center_pos, b2Color(0.0f, 1.0f, 0.0f));
      DebugDraw::drawCircle(target, center_pos, 0.15f, b2Color(0.0f, 1.0f, 0.0f));

      if (light->_center_offset_m.x != 0.0f || light->_center_offset_m.y != 0.0f)
      {
         DebugDraw::drawLine(target, base_pos, center_pos, b2Color(1.0f, 1.0f, 0.0f));
      }

      const auto sprite_center_px =
         sf::Vector2f(sprite_bounds.position.x + sprite_bounds.size.x * 0.5f, sprite_bounds.position.y + sprite_bounds.size.y * 0.5f);
      DebugDraw::drawPoint(target, sprite_center_px, b2Color(0.0f, 0.0f, 1.0f));
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
      _sprite->textureRect =
         sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(_texture->getSize().x), static_cast<float>(_texture->getSize().y)}};
      _sprite->scale = {static_cast<float>(_width_px) / _texture->getSize().x, static_cast<float>(_height_px) / _texture->getSize().y};
      _sprite->color = _color;
   }
}

void LightSystem::LightInstance::updateSpritePosition() const
{
   _sprite->position = sf::Vector2f(_pos_m.x * PPM - _width_px * 0.5f, _pos_m.y * PPM - _height_px * 0.5f);
}

std::shared_ptr<LightSystem::LightInstance> LightSystem::createLightInstance(GameNode* parent, const GameDeserializeData& data)
{
   auto light = std::make_shared<LightSystem::LightInstance>(parent);

   std::string texture = "data/light/smooth.png";

   if (data._tmx_object)
   {
      if (data._tmx_object->_width_px > 0)
      {
         light->_width_px = static_cast<int32_t>(data._tmx_object->_width_px);
      }

      if (data._tmx_object->_height_px > 0)
      {
         light->_height_px = static_cast<int32_t>(data._tmx_object->_height_px);
      }

      light->_pos_m = b2Vec2(
         (data._tmx_object->_x_px * MPP) + (data._tmx_object->_width_px * 0.5f * MPP),
         (data._tmx_object->_y_px * MPP) + (data._tmx_object->_height_px * 0.5f * MPP)
      );

      light->setObjectId(data._tmx_object->_name);

      if (data._tmx_object->_properties)
      {
         const auto& property_map = data._tmx_object->_properties->_map;

         if (const auto color_string = ValueReader::readValue<std::string>("color", property_map))
         {
            const auto color_channels = TmxTools::color(color_string.value());
            light->_color = sf::Color(color_channels[0], color_channels[1], color_channels[2], color_channels[3]);
         }

         if (const auto texture_name = ValueReader::readValue<std::string>("texture", property_map))
         {
            texture = (std::filesystem::path("data/light/") / texture_name.value()).string();
         }

         if (const auto center_offset_x = ValueReader::readValue<int32_t>("center_offset_x_px", property_map))
         {
            light->_center_offset_px.x = center_offset_x.value();
            light->_center_offset_m.x = center_offset_x.value() * MPP;
         }

         if (const auto center_offset_y = ValueReader::readValue<int32_t>("center_offset_y_px", property_map))
         {
            light->_center_offset_px.y = center_offset_y.value();
            light->_center_offset_m.y = center_offset_y.value() * MPP;
         }
      }
   }

   light->_texture = TexturePool::getInstance().get(texture);
   light->_sprite = std::make_unique<sf::Sprite>();
   light->_sprite->textureRect =
      sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(light->_texture->getSize().x), static_cast<float>(light->_texture->getSize().y)}};

   light->updateSpritePosition();

   const auto scale_x = static_cast<float>(light->_width_px) / static_cast<float>(light->_texture->getSize().x);
   const auto scale_y = static_cast<float>(light->_height_px) / static_cast<float>(light->_texture->getSize().y);
   light->_sprite->scale = {scale_x, scale_y};

   return light;
}

std::shared_ptr<LightSystem::LightInstance> LightSystem::createLightInstance(GameNode* parent, const nlohmann::json& node)
{
   auto light = std::make_shared<LightSystem::LightInstance>(parent);
   light->_texture = TexturePool::getInstance().get("data/light/smooth.png");
   light->_sprite = std::make_unique<sf::Sprite>();
   light->_sprite->textureRect =
      sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(light->_texture->getSize().x), static_cast<float>(light->_texture->getSize().y)}};
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
