#include "dust.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"



void Dust::update(const sf::Time& dt)
{
   // do flowfield lookup?
   // generate flowfield?

   const auto image = _flow_field.copyToImage();
   for (auto& p : _particles)
   {
      const auto x_px = p._position.x - _clip_rect.left;
      const auto y_px = p._position.y - _clip_rect.top;

      if (x_px < 0 || x_px >= _clip_rect.width)
      {
         p.spawn(_clip_rect);
         continue;
      }

      if (y_px < 0 || y_px >= _clip_rect.height)
      {
         p.spawn(_clip_rect);
         continue;
      }

      const auto col = image.getPixel(x_px, y_px);

      const auto col_x = (static_cast<float>(col.r) / 255.0f) - 0.5f;
      const auto col_y = (static_cast<float>(col.g) / 255.0f) - 0.5f;
      const auto col_z = (static_cast<float>(col.b) / 255.0f) - 0.5f;

      p._position = p._position + sf::Vector3f{col_x, col_y, col_z};
      p._z = col_z;

      p._age += dt.asSeconds();

      if (p._age > p._lifetime)
      {
         p.spawn(_clip_rect);
      }
   }
}



void Dust::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   for (const auto& p : _particles)
   {
      const auto pos = p._position;
      const auto val = 255;
      const auto alpha = 50 + p._z * 0.1f;
      const auto col = sf::Color{val, val, val, static_cast<uint8_t>((p._age > p._lifetime - 1.0f) ? ((p._lifetime - p._age) * alpha) : alpha)};

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

   dust->_clip_rect = sf::FloatRect {
      tmx_object->_x_px,
      tmx_object->_y_px,
      tmx_object->_width_px,
      tmx_object->_height_px
   };

   if (tmx_object->_properties)
   {
      const auto z_it          = tmx_object->_properties->_map.find("z");
      const auto drop_count_it = tmx_object->_properties->_map.find("particle_count");

      if (z_it != tmx_object->_properties->_map.end())
      {
         dust->setZ(z_it->second->_value_int.value());
      }

      if (drop_count_it != tmx_object->_properties->_map.end())
      {
         const auto particle_count = drop_count_it->second->_value_int.value();
         for (auto i = 0; i < particle_count; i++)
         {
            Particle p;
            p.spawn(dust->_clip_rect);
            dust->_particles.push_back(p);
         }
      }
   }

   dust->_flow_field.loadFromFile("data/effects/flowfield_3.png");

   return dust;
}


void Dust::Particle::spawn(sf::FloatRect& rect)
{
   _position.x = rect.left + std::rand() % static_cast<int32_t>(rect.width);
   _position.y = rect.top + std::rand() % static_cast<int32_t>(rect.height);
   _age = 0.0f;
   _lifetime = 5.0f + (std::rand() % 100) * 0.1f;
}


