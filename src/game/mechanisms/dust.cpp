#include "dust.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "game/io/texturepool.h"

Dust::Dust(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Dust).name());
}

void Dust::update(const sf::Time& dt)
{
   const auto dt_s = dt.asSeconds();

   for (auto& p : _particles)
   {
      const auto x_px = p._position.x - _clip_rect.position.x;
      const auto y_px = p._position.y - _clip_rect.position.y;

      if (x_px < 0 || x_px >= _clip_rect.size.x || y_px < 0 || y_px >= _clip_rect.size.y)
      {
         p.spawn(_clip_rect);
         continue;
      }

      const auto col = _flow_field_image.getPixel(static_cast<int32_t>(x_px), static_cast<int32_t>(y_px));
      const auto col_x = (static_cast<float>(col.r) / 255.0f) - 0.5f;
      const auto col_y = (static_cast<float>(col.g) / 255.0f) - 0.5f;
      const auto col_z = (static_cast<float>(col.b) / 255.0f) - 0.5f;
      const auto dir = sf::Vector3f{col_x, col_y, col_z};

      p._position = p._position + dir * dt_s * _particle_velocity + _wind_direction * dt_s * _particle_velocity;
      p._z = col_z;
      p._age += dt_s;

      if (p._age > p._lifetime)
      {
         p.spawn(_clip_rect);
      }
   }
}

void Dust::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   static const auto alpha_default = 50;

   sf::RenderStates states;
   states.blendMode = sf::BlendAlpha;

   sf::Vertex quad[4];
   for (const auto& p : _particles)
   {
      const auto pos = p._position;
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

      const auto col = sf::Color{_particle_color.r, _particle_color.g, _particle_color.b, static_cast<uint8_t>(alpha)};

      quad[0].position.x = pos.x;
      quad[0].position.y = pos.y;
      quad[1].position.x = pos.x;
      quad[1].position.y = pos.y + _particle_size_px;
      quad[2].position.x = pos.x + _particle_size_px;
      quad[2].position.y = pos.y + _particle_size_px;
      quad[3].position.x = pos.x + _particle_size_px;
      quad[3].position.y = pos.y;
      quad[0].color = col;
      quad[1].color = col;
      quad[2].color = col;
      quad[3].color = col;

      target.draw(quad, 4, sf::Quads, states);
   }
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
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   dust->_clip_rect = clip_rect;
   dust->addChunks(clip_rect);

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      const auto particle_size_it = data._tmx_object->_properties->_map.find("particle_size_px");
      const auto particle_count_it = data._tmx_object->_properties->_map.find("particle_count");
      const auto color_it = data._tmx_object->_properties->_map.find("particle_color");
      const auto velocity_it = data._tmx_object->_properties->_map.find("particle_velocity");
      const auto wind_dir_x_it = data._tmx_object->_properties->_map.find("wind_dir_x");
      const auto wind_dir_y_it = data._tmx_object->_properties->_map.find("wind_dir_y");
      const auto flowfield_texture_it = data._tmx_object->_properties->_map.find("flowfield_texture");

      if (z_it != data._tmx_object->_properties->_map.end())
      {
         dust->setZ(z_it->second->_value_int.value());
      }

      if (particle_size_it != data._tmx_object->_properties->_map.end())
      {
         dust->_particle_size_px = static_cast<uint8_t>(particle_size_it->second->_value_int.value());
      }

      if (particle_count_it != data._tmx_object->_properties->_map.end())
      {
         const auto particle_count = particle_count_it->second->_value_int.value();
         for (auto i = 0; i < particle_count; i++)
         {
            Particle p;
            p.spawn(dust->_clip_rect);
            dust->_particles.push_back(p);
         }
      }

      if (wind_dir_x_it != data._tmx_object->_properties->_map.end())
      {
         dust->_wind_direction.x = wind_dir_x_it->second->_value_float.value();
      }

      if (wind_dir_y_it != data._tmx_object->_properties->_map.end())
      {
         dust->_wind_direction.y = wind_dir_y_it->second->_value_float.value();
      }

      if (color_it != data._tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(color_it->second->_value_string.value());
         ;
         dust->_particle_color = {rgba[0], rgba[1], rgba[2]};
      }

      if (velocity_it != data._tmx_object->_properties->_map.end())
      {
         dust->_particle_velocity = velocity_it->second->_value_float.value();
      }

      if (flowfield_texture_it != data._tmx_object->_properties->_map.end())
      {
         flowfield_texture = flowfield_texture_it->second->_value_string.value();
      }
   }

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
}
