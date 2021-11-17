#include "dust.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"


void Dust::update(const sf::Time& dt)
{
   const auto dt_s = dt.asSeconds();

   for (auto& p : _particles)
   {
      const auto x_px = p._position.x - _clip_rect.left;
      const auto y_px = p._position.y - _clip_rect.top;

      if (x_px < 0 || x_px >= _clip_rect.width || y_px < 0 || y_px >= _clip_rect.height)
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

      sf::Vertex quad[] = {
         sf::Vertex(sf::Vector2f(pos.x,     pos.y    ), col),
         sf::Vertex(sf::Vector2f(pos.x,     pos.y + 2), col),
         sf::Vertex(sf::Vector2f(pos.x + 2, pos.y + 2), col),
         sf::Vertex(sf::Vector2f(pos.x + 2, pos.y    ), col)
      };

      sf::RenderStates states;
      states.blendMode = sf::BlendAlpha;
      target.draw(quad, 4, sf::Quads, states);
   }
}


std::shared_ptr<Dust> Dust::deserialize(TmxObject* tmx_object)
{
   auto dust = std::make_shared<Dust>();

   std::string flowfield_texture = "data/effects/flowfield_3.png";

   dust->_clip_rect = sf::FloatRect {
      tmx_object->_x_px,
      tmx_object->_y_px,
      tmx_object->_width_px,
      tmx_object->_height_px
   };

   if (tmx_object->_properties)
   {
      const auto z_it                 = tmx_object->_properties->_map.find("z");
      const auto particle_count_it    = tmx_object->_properties->_map.find("particle_count");
      const auto color_it             = tmx_object->_properties->_map.find("particle_color");
      const auto velocity_it          = tmx_object->_properties->_map.find("particle_velocity");
      const auto wind_dir_x_it        = tmx_object->_properties->_map.find("wind_dir_x");
      const auto wind_dir_y_it        = tmx_object->_properties->_map.find("wind_dir_y");
      const auto flowfield_texture_it = tmx_object->_properties->_map.find("flowfield_texture");

      if (z_it != tmx_object->_properties->_map.end())
      {
         dust->setZ(z_it->second->_value_int.value());
      }

      if (particle_count_it != tmx_object->_properties->_map.end())
      {
         const auto particle_count = particle_count_it->second->_value_int.value();
         for (auto i = 0; i < particle_count; i++)
         {
            Particle p;
            p.spawn(dust->_clip_rect);
            dust->_particles.push_back(p);
         }
      }

      if (wind_dir_x_it != tmx_object->_properties->_map.end())
      {
         dust->_wind_direction.x = wind_dir_x_it->second->_value_float.value();
      }

      if (wind_dir_y_it != tmx_object->_properties->_map.end())
      {
         dust->_wind_direction.y = wind_dir_y_it->second->_value_float.value();
      }

      if (color_it != tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(color_it->second->_value_string.value());;
         dust->_particle_color = {rgba[0], rgba[1], rgba[2]};
      }

      if (velocity_it != tmx_object->_properties->_map.end())
      {
         dust->_particle_velocity = velocity_it->second->_value_float.value();
      }

      if (flowfield_texture_it != tmx_object->_properties->_map.end())
      {
         flowfield_texture = flowfield_texture_it->second->_value_string.value();
      }
   }

   sf::Texture flow_field;
   flow_field.loadFromFile(flowfield_texture);
   dust->_flow_field_image = flow_field.copyToImage();

   return dust;
}


void Dust::Particle::spawn(sf::FloatRect& rect)
{
   _position.x = rect.left + std::rand() % static_cast<int32_t>(rect.width);
   _position.y = rect.top + std::rand() % static_cast<int32_t>(rect.height);
   _age = 0.0f;
   _lifetime = 5.0f + (std::rand() % 100) * 0.1f;
}


