#include "smokeeffect.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "texturepool.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <math.h>


namespace
{
sf::BlendMode blend_mode = sf::BlendAdd;
}


SmokeEffect::SmokeEffect()
 : _texture(TexturePool::getInstance().get("data/effects/smoke.png"))
{
   _texture->setSmooth(true);
}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates states, int z)
{
   if (z != _z)
   {
       return;
   }

   sf::RenderTexture render_texture;

   if (!render_texture.create(
         static_cast<int32_t>(_size_px.x / _pixel_ratio),
         static_cast<int32_t>(_size_px.y / _pixel_ratio)
      )
   )
   {
      return;
   }

   for (auto& particle : _particles)
   {
      render_texture.draw(particle._sprite, blend_mode);
   }

   render_texture.setSmooth(false);
   render_texture.display();

   sf::Sprite rt_sprite(render_texture.getTexture());
   rt_sprite.setPosition(_offset_px);
   rt_sprite.scale(_pixel_ratio, _pixel_ratio);
   rt_sprite.setColor({255, 255, 255, 150});

   target.draw(rt_sprite, states);
}


void SmokeEffect::update(const sf::Time& time)
{
   const auto dt = time.asSeconds() - _last_update_time.asSeconds();
   _last_update_time = time;

   for (auto& particle : _particles)
   {
      particle._rot += dt * 10.0f * particle._rot_dir;
      particle._sprite.setRotation(particle._rot);

      // fake z rotation
      const auto x = 0.5f * (1.0f + sin(particle._time_offset + time.asSeconds())) * particle._offset.x;
      const auto y = 0.5f * (1.0f + cos(particle._time_offset + time.asSeconds())) * particle._offset.y;

      particle._sprite.setPosition(particle._center.x + x, particle._center.y + y);

      // moved here from deserialize code
      // origin should always depend on rotation
      const auto bounds = particle._sprite.getGlobalBounds();
      particle._sprite.setOrigin(bounds.width/2, bounds.height/2);
   }
}


//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(TmxObject* tmx_object, TmxObjectGroup* /*objectGroup*/)
{
   auto smoke_effect = std::make_shared<SmokeEffect>();

   auto particle_count = 50;
   auto sprite_scale = 1.0f;
   auto spread_factor = 1.5f;
   sf::Color particle_color{255, 255, 255, 25};

   if (tmx_object->_properties)
   {
      auto z_it = tmx_object->_properties->_map.find("z");
      if (z_it != tmx_object->_properties->_map.end())
      {
         smoke_effect->_z = z_it->second->_value_int.value();
      }

      auto particle_count_it = tmx_object->_properties->_map.find("particle_count");
      if (particle_count_it != tmx_object->_properties->_map.end())
      {
         particle_count = particle_count_it->second->_value_int.value();
      }

      auto spread_factor_it = tmx_object->_properties->_map.find("spread_factor");
      if (spread_factor_it != tmx_object->_properties->_map.end())
      {
         spread_factor = spread_factor_it->second->_value_float.value();
      }

      auto sprite_scale_it = tmx_object->_properties->_map.find("sprite_scale");
      if (sprite_scale_it != tmx_object->_properties->_map.end())
      {
         sprite_scale = sprite_scale_it->second->_value_float.value();
      }

      auto pixel_ratio_it = tmx_object->_properties->_map.find("pixel_ratio");
      if (pixel_ratio_it != tmx_object->_properties->_map.end())
      {
         smoke_effect->_pixel_ratio = pixel_ratio_it->second->_value_float.value();
      }

      auto particle_color_it = tmx_object->_properties->_map.find("particle_color");
      if (particle_color_it != tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(particle_color_it->second->_value_string.value());
         particle_color = {rgba[0], rgba[1], rgba[2], rgba[3]};
      }
   }

   const auto rect_width_px = static_cast<int32_t>(tmx_object->_width_px);
   const auto rect_height_px = static_cast<int32_t>(tmx_object->_height_px);

   smoke_effect->_offset_px.x = tmx_object->_x_px;
   smoke_effect->_offset_px.y = tmx_object->_y_px;

   smoke_effect->_size_px.x = rect_width_px;
   smoke_effect->_size_px.y = rect_height_px;

   // define the range within the defined rectangle where particles will spawn
   const auto range_x = static_cast<int32_t>((rect_width_px / smoke_effect->_pixel_ratio) / spread_factor);
   const auto range_y = static_cast<int32_t>((rect_height_px / smoke_effect->_pixel_ratio) / spread_factor);

   for (auto i = 0; i < particle_count; i++)
   {
      SmokeParticle particle;
      auto x = static_cast<float>(std::rand() % range_x - range_x / 2); // normalize form (-range / 2) to (range / 2)
      auto y = static_cast<float>(std::rand() % range_y - range_y / 2);

      const auto rotation = static_cast<float>(std::rand() % 360);
      const auto time_offset = static_cast<float>(std::rand() % 100) * 0.01f * 2.0f * static_cast<float>(M_PI);

      const auto center_x_px = (tmx_object->_width_px / 2) / smoke_effect->_pixel_ratio;
      const auto center_y_px = (tmx_object->_height_px / 2) / smoke_effect->_pixel_ratio;

      const auto sprite_scale_x = (std::rand() % 50 + 50) * 0.004f * sprite_scale;
      const auto sprite_scale_y = (std::rand() % 50 + 50) * 0.004f * sprite_scale;

      particle._sprite.setScale(sprite_scale_x, sprite_scale_y);
      particle._sprite.setRotation(rotation);

      particle._rot_dir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;
      particle._center = sf::Vector2f{center_x_px, center_y_px};
      particle._offset = sf::Vector2f{x, y};
      particle._time_offset = time_offset;

      particle._sprite.setTexture(*smoke_effect->_texture);
      particle._sprite.setColor(particle_color);

//      // moved here from deserialize code
//      // origin should always depend on rotation
//      const auto bounds = particle._sprite.getGlobalBounds();
//      particle._sprite.setOrigin(bounds.width, bounds.height);

      smoke_effect->_particles.push_back(particle);
   }

   return smoke_effect;
}

