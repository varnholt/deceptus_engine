#include "smokeeffect.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "texturepool.h"

#include <math.h>
#include <array>
#include <filesystem>
#include <iostream>

SmokeEffect::SmokeEffect(GameNode* parent) : GameNode(parent), _texture(TexturePool::getInstance().get("data/effects/smoke.png"))
{
   setClassName(typeid(SmokeEffect).name());
   _texture->setSmooth(true);
   _z_index = 20;
}

void SmokeEffect::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   _render_texture.clear();

   for (auto& particle : _particles)
   {
      _render_texture.draw(particle._sprite, _blend_mode);
   }

   _render_texture.setSmooth(false);
   _render_texture.display();

   sf::Sprite rt_sprite(_render_texture.getTexture());
   rt_sprite.setPosition(_offset_px);
   rt_sprite.scale(_pixel_ratio, _pixel_ratio);
   rt_sprite.setColor(_layer_color);

   color.draw(rt_sprite);
}

void SmokeEffect::update(const sf::Time& dt)
{
   _elapsed += dt;
   const auto dt_scaled = dt.asSeconds() * _velocity;

   for (auto& particle : _particles)
   {
      particle._rot += dt_scaled * 10.0f * particle._rot_dir;
      particle._sprite.setRotation(particle._rot);

      // fake z rotation
      const auto x_normalized = 0.5f * (1.0f + sin(particle._time_offset + _elapsed.asSeconds() * _velocity));
      const auto y_normalized = 0.5f * (1.0f + cos(particle._time_offset + _elapsed.asSeconds() * _velocity));
      const auto x = x_normalized * particle._offset.x;
      const auto y = y_normalized * particle._offset.y;

      particle._sprite.setPosition(particle._center.x + x, particle._center.y + y);

      if (_mode == Mode::Fog)
      {
         particle._sprite.setColor(
            {_particle_color.r, _particle_color.g, _particle_color.b, static_cast<uint8_t>(_particle_color.a * fabs(x_normalized))}
         );
      }

      // moved here from deserialize code
      // origin should always depend on rotation
      const auto bounds = particle._sprite.getGlobalBounds();
      particle._sprite.setOrigin(bounds.width / 2, bounds.height / 2);
   }
}

//-----------------------------------------------------------------------------
std::optional<sf::FloatRect> SmokeEffect::getBoundingBoxPx()
{
   return _bounding_box_px;
}

//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto smoke_effect = std::make_shared<SmokeEffect>(parent);
   smoke_effect->setObjectId(data._tmx_object->_name);

   auto particle_count = 50;
   auto sprite_scale = 1.0f;
   auto spread_factor = 1.0f / 1.5f;
   auto center_offset_x_px = 0;
   auto center_offset_y_px = 0;

   if (data._tmx_object->_properties)
   {
      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         smoke_effect->_z_index = z_it->second->_value_int.value();
      }

      auto particle_count_it = data._tmx_object->_properties->_map.find("particle_count");
      if (particle_count_it != data._tmx_object->_properties->_map.end())
      {
         particle_count = particle_count_it->second->_value_int.value();
      }

      auto spread_factor_it = data._tmx_object->_properties->_map.find("spread_factor");
      if (spread_factor_it != data._tmx_object->_properties->_map.end())
      {
         spread_factor = spread_factor_it->second->_value_float.value();
      }

      auto velocity_it = data._tmx_object->_properties->_map.find("velocity");
      if (velocity_it != data._tmx_object->_properties->_map.end())
      {
         smoke_effect->_velocity = velocity_it->second->_value_float.value();
      }

      auto sprite_scale_it = data._tmx_object->_properties->_map.find("sprite_scale");
      if (sprite_scale_it != data._tmx_object->_properties->_map.end())
      {
         sprite_scale = sprite_scale_it->second->_value_float.value();
      }

      auto pixel_ratio_it = data._tmx_object->_properties->_map.find("pixel_ratio");
      if (pixel_ratio_it != data._tmx_object->_properties->_map.end())
      {
         smoke_effect->_pixel_ratio = pixel_ratio_it->second->_value_float.value();
      }

      auto particle_color_it = data._tmx_object->_properties->_map.find("particle_color");
      if (particle_color_it != data._tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(particle_color_it->second->_value_string.value());
         smoke_effect->_particle_color = {rgba[0], rgba[1], rgba[2], rgba[3]};
      }

      auto layer_color_it = data._tmx_object->_properties->_map.find("layer_color");
      if (layer_color_it != data._tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(layer_color_it->second->_value_string.value());
         smoke_effect->_layer_color = {rgba[0], rgba[1], rgba[2], rgba[3]};
      }

      auto center_offset_x_it = data._tmx_object->_properties->_map.find("center_offset_x_px");
      if (center_offset_x_it != data._tmx_object->_properties->_map.end())
      {
         center_offset_x_px = center_offset_x_it->second->_value_int.value();
      }

      auto center_offset_y_it = data._tmx_object->_properties->_map.find("center_offset_y_px");
      if (center_offset_y_it != data._tmx_object->_properties->_map.end())
      {
         center_offset_y_px = center_offset_y_it->second->_value_int.value();
      }

      auto mode_it = data._tmx_object->_properties->_map.find("mode");
      if (mode_it != data._tmx_object->_properties->_map.end())
      {
         const auto mode = mode_it->second->_value_string.value();
         if (mode == "smoke")
         {
            smoke_effect->_mode = Mode::Smoke;
         }
         else if (mode == "fog")
         {
            smoke_effect->_mode = Mode::Fog;
         }
      }

      auto blend_mode_it = data._tmx_object->_properties->_map.find("blend_mode");
      if (blend_mode_it != data._tmx_object->_properties->_map.end())
      {
         const auto blend_mode = blend_mode_it->second->_value_string.value();
         if (blend_mode == "add")
         {
            smoke_effect->_blend_mode = sf::BlendAdd;
         }
         else if (blend_mode == "alpha")
         {
            smoke_effect->_blend_mode = sf::BlendAlpha;
         }
         else if (blend_mode == "multiply")
         {
            smoke_effect->_blend_mode = sf::BlendMultiply;
         }
      }
   }

   const auto rect_width_px = static_cast<int32_t>(data._tmx_object->_width_px);
   const auto rect_height_px = static_cast<int32_t>(data._tmx_object->_height_px);

   smoke_effect->_offset_px.x = data._tmx_object->_x_px;
   smoke_effect->_offset_px.y = data._tmx_object->_y_px;
   smoke_effect->_size_px.x = rect_width_px;
   smoke_effect->_size_px.y = rect_height_px;

   smoke_effect->_bounding_box_px.left = data._tmx_object->_x_px;
   smoke_effect->_bounding_box_px.top = data._tmx_object->_y_px;
   smoke_effect->_bounding_box_px.width = rect_width_px;
   smoke_effect->_bounding_box_px.height = rect_height_px;

   // also use the bounding box to compute the effect's chunks
   smoke_effect->addChunks(smoke_effect->_bounding_box_px);

   // define the range within the defined rectangle where particles will spawn
   const auto range_x = static_cast<int32_t>((rect_width_px / smoke_effect->_pixel_ratio) * spread_factor);
   const auto range_y = static_cast<int32_t>((rect_height_px / smoke_effect->_pixel_ratio) * spread_factor);

   const auto center_x_px = center_offset_x_px + (data._tmx_object->_width_px / 2) / smoke_effect->_pixel_ratio;
   const auto center_y_px = center_offset_y_px + (data._tmx_object->_height_px / 2) / smoke_effect->_pixel_ratio;

   for (auto i = 0; i < particle_count; i++)
   {
      SmokeParticle particle;

      const auto offset_x_px = static_cast<float>((std::rand() % range_x) - range_x / 2);  // normalize from (-range / 2) to (range / 2)
      const auto offset_y_px = static_cast<float>((std::rand() % range_y) - range_y / 2);

      const auto sprite_scale_x = ((std::rand() % 50) + 50) * 0.004f * sprite_scale;
      const auto sprite_scale_y = ((std::rand() % 50) + 50) * 0.004f * sprite_scale;

      particle._rot_dir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;  // -1.0 .. 1.0
      particle._center = sf::Vector2f{center_x_px, center_y_px};
      particle._offset = sf::Vector2f{offset_x_px, offset_y_px};
      particle._time_offset = static_cast<float>(std::rand() % 100) * 0.02f * static_cast<float>(M_PI);  // 0 .. 2_PI

      particle._sprite.setScale(sprite_scale_x, sprite_scale_y);
      particle._sprite.setRotation(static_cast<float>(std::rand() % 360));
      particle._sprite.setTexture(*smoke_effect->_texture);
      particle._sprite.setColor(smoke_effect->_particle_color);

      smoke_effect->_particles.push_back(particle);
   }

   if (!smoke_effect->_render_texture.create(
          static_cast<int32_t>(rect_width_px / smoke_effect->_pixel_ratio),
          static_cast<int32_t>(rect_height_px / smoke_effect->_pixel_ratio)
       ))
   {
      Log::Error() << "could not create render texture for smoke effect";
      return nullptr;
   }

   return smoke_effect;
}
