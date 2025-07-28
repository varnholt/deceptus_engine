#include "dust.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "game/event/eventdistributor.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/flowfieldtexturechangeevent.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
const auto registered_dust = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("Dust", "dust");

   registry.registerLayerName(
      "dust",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = Dust::deserialize(parent, data);
         mechanisms["dust"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "Dust",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = Dust::deserialize(parent, data);
         mechanisms["dust"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

Dust::Dust(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Dust).name());
   _vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
}

Dust::~Dust()
{
   if (_flowfield_listener_id.has_value())
   {
      EventDistributor::unregisterEvent<FlowFieldTextureChangeEvent>(_flowfield_listener_id.value());
   }
}

void Dust::update(const sf::Time& dt)
{
   const auto dt_s = dt.asSeconds();

   const auto scale_factor_x = static_cast<float>(_flow_field_image.getSize().x) / _clip_rect.size.x;
   const auto scale_factor_y = static_cast<float>(_flow_field_image.getSize().y) / _clip_rect.size.y;

   for (auto& p : _particles)
   {
      const auto x_px = p._position.x - _clip_rect.position.x;
      const auto y_px = p._position.y - _clip_rect.position.y;

      if (x_px < 0 || x_px >= _clip_rect.size.x || y_px < 0 || y_px >= _clip_rect.size.y)
      {
         p.spawn(_clip_rect);
         continue;
      }

      const auto col =
         _flow_field_image.getPixel({static_cast<uint32_t>(x_px * scale_factor_x), static_cast<uint32_t>(y_px * scale_factor_y)});
      const auto col_x = (static_cast<float>(col.r) / 255.0f) - 0.5f;
      const auto col_y = (static_cast<float>(col.g) / 255.0f) - 0.5f;
      const auto col_z = (static_cast<float>(col.b) / 255.0f) - 0.5f;
      const auto dir = sf::Vector3f{col_x, col_y, col_z};

      const auto position_prev = p._position;
      p._position = p._position + dir * dt_s * _particle_velocity + _wind_direction * dt_s * _particle_velocity;
      p._z = col_z;
      p._age += dt_s;

      if (p._age > p._lifetime)
      {
         p.spawn(_clip_rect);
         continue;
      }

      // remove particles that are too close to the center
      if (_respawn_when_center_reached)
      {
         const sf::Vector2f center = _clip_rect.getCenter();
         const auto dx = p._position.x - center.x;
         const auto dy = p._position.y - center.y;
         const auto dist_sq = dx * dx + dy * dy;
         const auto too_close_to_center = dist_sq < p._center_reset_radius_sq;

         if (too_close_to_center)
         {
            p.spawn(_clip_rect);
            continue;
         }
      }
   }
}

void Dust::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   static const auto alpha_default = 50;

   sf::RenderStates states;
   states.blendMode = sf::BlendAlpha;

   std::size_t vertex_index = 0;

   sf::Vertex quad[4];
   for (const auto& p : _particles)
   {
      const auto pos_px = p._position;
      auto alpha = 0.0f;

      if (p._age > p._lifetime - 1.0f)
      {
         alpha = (p._lifetime - p._age) * alpha_default;
      }
      else if (p._age < 1.0f)
      {
         alpha = p._age * alpha_default;
      }
      else
      {
         alpha = alpha_default + p._z * 50.0f;
      }

      const auto color =
         sf::Color{_particle_color.r, _particle_color.g, _particle_color.b, static_cast<uint8_t>(std::clamp(alpha, 0.0f, 255.0f))};

      if (vertex_index + 6 > _vertices.getVertexCount())
      {
         break;
      }

      // triangle 1: top-left, top-right, bottom-right
      _vertices[vertex_index + 0] = sf::Vertex({pos_px.x, pos_px.y}, color);
      _vertices[vertex_index + 1] = sf::Vertex({pos_px.x + _particle_size_px, pos_px.y}, color);
      _vertices[vertex_index + 2] = sf::Vertex({pos_px.x + _particle_size_px, pos_px.y + _particle_size_px}, color);

      // triangle 2: top-left, bottom-right, bottom-left
      _vertices[vertex_index + 3] = sf::Vertex({pos_px.x, pos_px.y}, color);
      _vertices[vertex_index + 4] = sf::Vertex({pos_px.x + _particle_size_px, pos_px.y + _particle_size_px}, color);
      _vertices[vertex_index + 5] = sf::Vertex({pos_px.x, pos_px.y + _particle_size_px}, color);

      vertex_index += 6;

      // old approach where each dust particle was drawn individually
      //
      // quad[0].position = {pos.x, pos.y};                                          // bottom-left
      // quad[1].position = {pos.x, pos.y + _particle_size_px};                      // top-left
      // quad[2].position = {pos.x + _particle_size_px, pos.y};                      // bottom-right
      // quad[3].position = {pos.x + _particle_size_px, pos.y + _particle_size_px};  // top-right
      //
      // for (auto& v : quad)
      // {
      //    v.color = color;
      // }
      //
      // target.draw(quad, 4, sf::PrimitiveType::TriangleStrip, states);
   }

   target.draw(&_vertices[0], vertex_index, sf::PrimitiveType::Triangles, states);
}

std::optional<sf::FloatRect> Dust::getBoundingBoxPx()
{
   return _clip_rect;
}

std::shared_ptr<Dust> Dust::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto dust = std::make_shared<Dust>(parent);
   dust->setObjectId(data._tmx_object->_name);

   std::string flowfield_texture = "data/effects/flowfield_3.png";

   const auto clip_rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   dust->_clip_rect = clip_rect;
   dust->addChunks(clip_rect);

   auto particle_count = 100;

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;

      const auto z_it = map.find("z");
      const auto particle_size_it = map.find("particle_size_px");
      const auto particle_count_it = map.find("particle_count");
      const auto color_it = map.find("particle_color");
      const auto velocity_it = map.find("particle_velocity");
      const auto wind_dir_x_it = map.find("wind_dir_x");
      const auto wind_dir_y_it = map.find("wind_dir_y");
      const auto flowfield_texture_it = map.find("flowfield_texture");

      dust->_respawn_when_center_reached = ValueReader::readValue<bool>("respawn_when_center_reached", map).value_or(false);
      const auto allow_texture_updates = ValueReader::readValue<bool>("allow_texture_updates", map).value_or(false);

      if (z_it != map.end())
      {
         dust->setZ(z_it->second->_value_int.value());
      }

      if (particle_size_it != map.end())
      {
         dust->_particle_size_px = static_cast<uint8_t>(particle_size_it->second->_value_int.value());
      }

      if (particle_count_it != map.end())
      {
         particle_count = particle_count_it->second->_value_int.value();
      }

      if (wind_dir_x_it != map.end())
      {
         dust->_wind_direction.x = wind_dir_x_it->second->_value_float.value();
      }

      if (wind_dir_y_it != map.end())
      {
         dust->_wind_direction.y = wind_dir_y_it->second->_value_float.value();
      }

      if (color_it != map.end())
      {
         const auto rgba = TmxTools::color(color_it->second->_value_string.value());
         dust->_particle_color = {rgba[0], rgba[1], rgba[2]};
      }

      if (velocity_it != map.end())
      {
         dust->_particle_velocity = velocity_it->second->_value_float.value();
      }

      if (flowfield_texture_it != map.end())
      {
         flowfield_texture = flowfield_texture_it->second->_value_string.value();
      }

      if (allow_texture_updates)
      {
         dust->_flowfield_listener_id = EventDistributor::registerEvent<FlowFieldTextureChangeEvent>(
            [dust](const FlowFieldTextureChangeEvent& event)
            {
               if (event._object_id == dust->_object_id)
               {
                  dust->_flow_field_texture = TexturePool::getInstance().get(event._texture_id);
                  dust->_flow_field_image = dust->_flow_field_texture->copyToImage();
               }
            }
         );
      }
   }

   // generate dust vertices
   for (auto i = 0; i < particle_count; i++)
   {
      Particle p;
      p.spawn(dust->_clip_rect);
      dust->_particles.push_back(p);
   }

   dust->_vertices.resize(6 * particle_count);

   // remember the instance so it's not instantly removed from the cache and each
   // dust instance has to reload the texture
   dust->_flow_field_texture = TexturePool::getInstance().get(flowfield_texture);
   dust->_flow_field_image = dust->_flow_field_texture->copyToImage();

   return dust;
}

void Dust::Particle::spawn(sf::FloatRect& rect)
{
   _position.x = rect.position.x + std::rand() % static_cast<int32_t>(rect.size.x);
   _position.y = rect.position.y + std::rand() % static_cast<int32_t>(rect.size.y);
   _age = 0.0f;
   _lifetime = 5.0f + (std::rand() % 100) * 0.1f;

   constexpr auto radius_min = 1.0f;
   constexpr auto radius_max = 4.0f;
   const auto radius = radius_min + (std::rand() % 1000 / 1000.0f) * (radius_max - radius_min);
   _center_reset_radius_sq = radius * radius;
}
