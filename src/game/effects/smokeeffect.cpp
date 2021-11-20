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
constexpr auto pixel_ratio = 2.0f;
//int32_t count = 0;
//bool done = false;
}


SmokeEffect::SmokeEffect()
 : _texture(TexturePool::getInstance().get("data/effects/smoke.png"))
{
   _texture->setSmooth(true);
}



//void SmokeEffect::drawToZV1(sf::RenderTarget &target, sf::RenderStates /*states*/, int z)
//{
//    if (z != 20)
//    {
//        return;
//    }
//
//   for (auto& particle : _particles)
//   {
//      target.draw(particle._sprite, blend_mode);
//   }
//}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates states, int z)
{
   if (z != 20)
   {
       return;
   }

   sf::RenderTexture render_texture;

   //if (!render_texture.create(target.getSize().x, target.getSize().y))
   if (!render_texture.create(_size_px.x / pixel_ratio, _size_px.y / pixel_ratio))
   {
      return;
   }

//   render_texture.clear({255, 0, 0, 100});

   for (auto& particle : _particles)
   {
      render_texture.draw(particle._sprite, blend_mode);
   }

//   if (count % 64 == 0)
//   {
//      render_texture.getTexture().copyToImage().saveToFile("bla.png");
//      done = true;
//   }
//   count++;

   render_texture.setSmooth(false);
   render_texture.display();

   sf::Sprite rt_sprite(render_texture.getTexture());
   rt_sprite.setPosition(_offset_px);
   rt_sprite.scale(pixel_ratio, pixel_ratio);
   rt_sprite.setColor({255, 255, 255, 150});

//   states.blendMode = blend_mode;
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
      particle._sprite.setOrigin(bounds.width, bounds.height);
   }
}


//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(TmxObject* tmx_object, TmxObjectGroup* /*objectGroup*/)
{
   auto smoke_effect = std::make_shared<SmokeEffect>();

   if (tmx_object->_properties)
   {
      auto z = tmx_object->_properties->_map.find("z");
      if (z != tmx_object->_properties->_map.end())
      {
         smoke_effect->_z = tmx_object->_properties->_map["z"]->_value_int.value();
      }
   }

   const auto rect_width_px = static_cast<int32_t>(tmx_object->_width_px);
   const auto rect_height_px = static_cast<int32_t>(tmx_object->_height_px);

   smoke_effect->_offset_px.x = tmx_object->_x_px;
   smoke_effect->_offset_px.y = tmx_object->_y_px;

   smoke_effect->_size_px.x = rect_width_px;
   smoke_effect->_size_px.y = rect_height_px;

   const auto spread_factor = 1.5f;

   // define the range within the defined rectangle where particles will spawn
   const auto range_x = static_cast<int32_t>((rect_width_px / pixel_ratio) / spread_factor);
   const auto range_y = static_cast<int32_t>((rect_height_px / pixel_ratio) / spread_factor);

   for (auto& particle : smoke_effect->_particles)
   {
      auto x = static_cast<float>(std::rand() % range_x - range_x / 2); // normalize form -range/2 .. range/2
      auto y = static_cast<float>(std::rand() % range_y - range_y / 2);

      const auto rotation = static_cast<float>(std::rand() % 360);
      const auto time_offset = static_cast<float>(std::rand() % 100) * 0.01f * 2.0f * static_cast<float>(M_PI);

      const auto center_x_px = (tmx_object->_width_px / 2) / pixel_ratio;
      const auto center_y_px = (tmx_object->_height_px / 2) / pixel_ratio;

      const auto sprite_scale_x = (std::rand() % 50 + 50) * 0.004f;
      const auto sprite_scale_y = (std::rand() % 50 + 50) * 0.004f;

      particle._sprite.setScale(sprite_scale_x, sprite_scale_y);
      particle._sprite.setRotation(rotation);

      particle._rot_dir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;
      particle._center = sf::Vector2f{center_x_px, center_y_px};
      particle._offset = sf::Vector2f{x, y};
      particle._time_offset = time_offset;

      particle._sprite.setTexture(*smoke_effect->_texture);
      particle._sprite.setColor(sf::Color(255, 255, 255, 25));
   }

   return smoke_effect;
}


//-----------------------------------------------------------------------------
//std::shared_ptr<SmokeEffect> SmokeEffect::deserializeV1(TmxObject* tmx_object, TmxObjectGroup* /*object_group*/)
//{
//   auto smoke_effect = std::make_shared<SmokeEffect>();
//
//   if (tmx_object->_properties)
//   {
//      auto z = tmx_object->_properties->_map.find("z");
//      if (z != tmx_object->_properties->_map.end())
//      {
//         smoke_effect->_z = tmx_object->_properties->_map["z"]->_value_int.value();
//      }
//   }
//
//   const auto rect_width_px = static_cast<int32_t>(tmx_object->_width_px);
//   const auto rect_height_px = static_cast<int32_t>(tmx_object->_height_px);
//
//   smoke_effect->_offset_px.x = tmx_object->_x_px;
//   smoke_effect->_offset_px.y = tmx_object->_y_px;
//
//   smoke_effect->_size_px.x = rect_width_px;
//   smoke_effect->_size_px.y = rect_height_px;
//
//   for (auto& particle : smoke_effect->_particles)
//   {
//      auto x = static_cast<float>(std::rand() % rect_width_px - rect_width_px / 2);
//      auto y = static_cast<float>(std::rand() % rect_height_px - rect_height_px / 2);
//      const auto rotation = static_cast<float>(std::rand() % 360);
//      const auto time_offset = static_cast<float>(std::rand() % 100) * 0.01f * 2.0f * static_cast<float>(M_PI);
//
//      const auto center_x_px = tmx_object->_x_px + tmx_object->_width_px / 2;
//      const auto center_y_px = tmx_object->_y_px + tmx_object->_height_px / 2;
//
//      const auto sx = (std::rand() % 50 + 50) * 0.008f; // scale from 0..0.4
//      const auto sy = (std::rand() % 50 + 50) * 0.008f;
//
//      particle._sprite.setScale(sx, sy);
//      particle._sprite.setRotation(rotation);
//      particle._sprite.setPosition(x + center_x_px, y + center_y_px);
//
//      particle._rot_dir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;
//      particle._center = sf::Vector2f{center_x_px, center_y_px};
//      particle._offset = sf::Vector2f{x, y};
//      particle._time_offset = time_offset;
//
//      particle._sprite.setTexture(*smoke_effect->_texture);
//      particle._sprite.setColor(sf::Color(255, 255, 255, 25));
//
//      const auto bounds = particle._sprite.getGlobalBounds();
//      particle._sprite.setOrigin(bounds.width * sx, bounds.height * sy);
//   }
//
//   return smoke_effect;
//}
