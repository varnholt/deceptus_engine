#include "staticlight.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/math/fbm.h"
#include "texturepool.h"

#include <array>
#include <filesystem>


const std::string StaticLight::__layer_name = "static_lights";


void StaticLight::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int z) const
{
   for (const auto& light : _lights)
   {
      if (z != light->_z)
      {
         continue;
      }

      auto lumen =
         fbm::mix(
           light->_color.a,
           light->_flicker_amount * 255.0f,
           1.0f - light->_flicker_alpha_amount
         );

      sf::Color color{
         light->_color.r,
         light->_color.g,
         light->_color.b,
         static_cast<sf::Uint8>(lumen)
      };

      light->_sprite.setColor(color);
      target.draw(light->_sprite, light->_blend_mode);
   }
}


void StaticLight::update(const sf::Time& time)
{
   auto y = 0;
   for (auto& light : _lights)
   {
      light->_flicker_amount =
          light->_flicker_intensity
        * fbm::fbm(
            fbm::vec2{
              y + time.asSeconds() * light->_flicker_speed,
              light->_time_offset + y / static_cast<float>(_lights.size())
          }
        );

      y++;
   }
}


//-----------------------------------------------------------------------------
std::shared_ptr<StaticLight::LightInstance> StaticLight::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto light = std::make_shared<StaticLight::LightInstance>(parent);
   light->setObjectName(data._tmx_object->_name);

   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";
   auto flicker_intensity = 0.0f;
   auto flicker_alpha_amount = 1.0f;
   auto flicker_speed = 0.0f;

   if (data._tmx_object->_properties != nullptr)
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

      it = data._tmx_object->_properties->_map.find("flicker_intensity");
      if (it != data._tmx_object->_properties->_map.end())
      {
         flicker_intensity = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("flicker_alpha_amount");
      if (it != data._tmx_object->_properties->_map.end())
      {
         flicker_alpha_amount = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("flicker_speed");
      if (it != data._tmx_object->_properties->_map.end())
      {
         flicker_speed = it->second->_value_float.value();
      }
   }

   light->_color.r = rgba[0];
   light->_color.g = rgba[1];
   light->_color.b = rgba[2];
   light->_color.a = rgba[3];
   light->_flicker_intensity = flicker_intensity;
   light->_flicker_alpha_amount = flicker_alpha_amount;
   light->_flicker_speed = flicker_speed;
   light->_sprite.setColor(light->_color);
   light->_texture = TexturePool::getInstance().get(texture);
   light->_sprite.setTexture(*light->_texture);
   light->_sprite.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px);
   light->_z = data._tmx_object_group->_z_index;

   auto scale_x_px = data._tmx_object->_width_px / light->_texture->getSize().x;
   auto scale_y_px = data._tmx_object->_height_px / light->_texture->getSize().y;
   light->_sprite.scale(scale_x_px, scale_y_px);

   // init each light with a different time offset
   // probably passing the position itself to FBM would be enough
   std::srand(static_cast<uint32_t>(data._tmx_object->_x_px * data._tmx_object->_y_px));
   light->_time_offset = (std::rand() % 100) * 0.01f;

   return light;
}

