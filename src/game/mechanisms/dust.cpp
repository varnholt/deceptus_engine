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

std::string_view Dust::objectName() const
{
   return "Dust";
}

void Dust::rebuildFlowFieldCache()
{
   const auto image_size = _flow_field_image.getSize();
   _flow_field_image_width = image_size.x;
   _flow_field_scale_factor_x = static_cast<float>(image_size.x) / _clip_rect.size.x;
   _flow_field_scale_factor_y = static_cast<float>(image_size.y) / _clip_rect.size.y;

   _flow_field_cache.resize(image_size.x * image_size.y);

   for (uint32_t image_pixel_y = 0; image_pixel_y < image_size.y; ++image_pixel_y)
   {
      for (uint32_t image_pixel_x = 0; image_pixel_x < image_size.x; ++image_pixel_x)
      {
         const auto image_pixel = _flow_field_image.getPixel({image_pixel_x, image_pixel_y});
         const auto direction_x = (static_cast<float>(image_pixel.r) / 255.0f) - 0.5f;
         const auto direction_y = (static_cast<float>(image_pixel.g) / 255.0f) - 0.5f;
         const auto direction_z = (static_cast<float>(image_pixel.b) / 255.0f) - 0.5f;
         _flow_field_cache[image_pixel_y * image_size.x + image_pixel_x] = sf::Vector3f{direction_x, direction_y, direction_z};
      }
   }
}

void Dust::update(const sf::Time& dt)
{
   const auto delta_s = dt.asSeconds();

   for (auto& particle : _particles)
   {
      const auto clip_relative_x_px = particle._position.x - _clip_rect.position.x;
      const auto clip_relative_y_px = particle._position.y - _clip_rect.position.y;

      if (clip_relative_x_px < 0 || clip_relative_x_px >= _clip_rect.size.x || clip_relative_y_px < 0 ||
          clip_relative_y_px >= _clip_rect.size.y)
      {
         particle.spawn(_clip_rect);
         continue;
      }

      const auto flow_field_pixel_x = static_cast<uint32_t>(clip_relative_x_px * _flow_field_scale_factor_x);
      const auto flow_field_pixel_y = static_cast<uint32_t>(clip_relative_y_px * _flow_field_scale_factor_y);
      const auto flow_direction = _flow_field_cache[flow_field_pixel_y * _flow_field_image_width + flow_field_pixel_x];

      particle._position =
         particle._position + flow_direction * delta_s * _particle_velocity + _wind_direction * delta_s * _particle_velocity;
      particle._z = flow_direction.z;
      particle._age += delta_s;

      if (particle._age > particle._lifetime)
      {
         particle.spawn(_clip_rect);
         continue;
      }

      // remove particles that are too close to the center
      if (_respawn_when_center_reached)
      {
         const sf::Vector2f center = _clip_rect.getCenter();
         const auto center_delta_x = particle._position.x - center.x;
         const auto center_delta_y = particle._position.y - center.y;
         const auto center_distance_sq = center_delta_x * center_delta_x + center_delta_y * center_delta_y;
         const auto too_close_to_center = center_distance_sq < particle._center_reset_radius_sq;

         if (too_close_to_center)
         {
            particle.spawn(_clip_rect);
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
   for (const auto& particle : _particles)
   {
      const auto particle_position_px = particle._position;
      auto particle_alpha = 0.0f;

      if (particle._age > particle._lifetime - 1.0f)
      {
         particle_alpha = (particle._lifetime - particle._age) * alpha_default;
      }
      else if (particle._age < 1.0f)
      {
         particle_alpha = particle._age * alpha_default;
      }
      else
      {
         particle_alpha = alpha_default + particle._z * 50.0f;
      }

      const auto color =
         sf::Color{_particle_color.r, _particle_color.g, _particle_color.b, static_cast<uint8_t>(std::clamp(particle_alpha, 0.0f, 255.0f))};

      if (vertex_index + 6 > _vertices.getVertexCount())
      {
         break;
      }

      // triangle 1: top-left, top-right, bottom-right
      _vertices[vertex_index + 0] = sf::Vertex({particle_position_px.x, particle_position_px.y}, color);
      _vertices[vertex_index + 1] = sf::Vertex({particle_position_px.x + _particle_size_px, particle_position_px.y}, color);
      _vertices[vertex_index + 2] =
         sf::Vertex({particle_position_px.x + _particle_size_px, particle_position_px.y + _particle_size_px}, color);

      // triangle 2: top-left, bottom-right, bottom-left
      _vertices[vertex_index + 3] = sf::Vertex({particle_position_px.x, particle_position_px.y}, color);
      _vertices[vertex_index + 4] =
         sf::Vertex({particle_position_px.x + _particle_size_px, particle_position_px.y + _particle_size_px}, color);
      _vertices[vertex_index + 5] = sf::Vertex({particle_position_px.x, particle_position_px.y + _particle_size_px}, color);

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
      const auto& properties_map = data._tmx_object->_properties->_map;

      const auto z_iterator = properties_map.find("z");
      const auto particle_size_iterator = properties_map.find("particle_size_px");
      const auto particle_count_iterator = properties_map.find("particle_count");
      const auto color_iterator = properties_map.find("particle_color");
      const auto velocity_iterator = properties_map.find("particle_velocity");
      const auto wind_dir_x_iterator = properties_map.find("wind_dir_x");
      const auto wind_dir_y_iterator = properties_map.find("wind_dir_y");
      const auto flowfield_texture_iterator = properties_map.find("flowfield_texture");

      dust->_respawn_when_center_reached = ValueReader::readValue<bool>("respawn_when_center_reached", properties_map).value_or(false);
      const auto allow_texture_updates = ValueReader::readValue<bool>("allow_texture_updates", properties_map).value_or(false);

      if (z_iterator != properties_map.end())
      {
         dust->setZ(z_iterator->second->_value_int.value());
      }

      if (particle_size_iterator != properties_map.end())
      {
         dust->_particle_size_px = static_cast<uint8_t>(particle_size_iterator->second->_value_int.value());
      }

      if (particle_count_iterator != properties_map.end())
      {
         particle_count = particle_count_iterator->second->_value_int.value();
      }

      if (wind_dir_x_iterator != properties_map.end())
      {
         dust->_wind_direction.x = wind_dir_x_iterator->second->_value_float.value();
      }

      if (wind_dir_y_iterator != properties_map.end())
      {
         dust->_wind_direction.y = wind_dir_y_iterator->second->_value_float.value();
      }

      if (color_iterator != properties_map.end())
      {
         const auto particle_color_components = TmxTools::color(color_iterator->second->_value_string.value());
         dust->_particle_color = {particle_color_components[0], particle_color_components[1], particle_color_components[2]};
      }

      if (velocity_iterator != properties_map.end())
      {
         dust->_particle_velocity = velocity_iterator->second->_value_float.value();
      }

      if (flowfield_texture_iterator != properties_map.end())
      {
         flowfield_texture = flowfield_texture_iterator->second->_value_string.value();
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
                  dust->rebuildFlowFieldCache();
               }
            }
         );
      }
   }

   // generate dust vertices
   for (auto particle_index = 0; particle_index < particle_count; particle_index++)
   {
      Particle new_particle;
      new_particle.spawn(dust->_clip_rect);
      dust->_particles.push_back(new_particle);
   }

   dust->_vertices.resize(6 * particle_count);

   // remember the instance so it's not instantly removed from the cache and each
   // dust instance has to reload the texture
   dust->_flow_field_texture = TexturePool::getInstance().get(flowfield_texture);
   dust->_flow_field_image = dust->_flow_field_texture->copyToImage();
   dust->rebuildFlowFieldCache();

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
