#include "ropewithlight.h"

#include "level.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"


RopeWithLight::RopeWithLight(GameNode* parent)
 : Rope(parent)
{
   setClassName(typeid(RopeWithLight).name());
}


void RopeWithLight::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   Rope::draw(color, normal);

   color.draw(_lamp_sprite);
}


void RopeWithLight::update(const sf::Time& dt)
{
   Rope::update(dt);

   _light->_pos_m = _chain_elements.back()->GetPosition();
   _light->updateSpritePosition();

   const auto c1_pos_m = _chain_elements[_chain_elements.size() - 2]->GetPosition();
   const auto c2_pos_m = _chain_elements[_chain_elements.size() - 1]->GetPosition();
   const auto c_m = (c1_pos_m - c2_pos_m);

   const auto angle_rad = static_cast<float>(atan2(c_m.y, c_m.x));

   _lamp_sprite.setRotation(90 + FACTOR_RAD_TO_DEG * angle_rad);
   _lamp_sprite.setPosition(
      _light->_pos_m.x * PPM, // - _lamp_sprite_rect.width / 2,
      _light->_pos_m.y * PPM  // - _lamp_sprite_rect.height / 2
   );
}


void RopeWithLight::setup(const GameDeserializeData& data)
{
   Rope::setup(data);

   // set up texture
   _lamp_sprite.setTexture(*_texture);

   // cut off 1st 4 pixels of the texture rect since there's some rope pixels in the spriteset
   _lamp_sprite_rect_1 = sf::IntRect{1056, 28, 24, 28};
   _lamp_sprite_rect_2 = sf::IntRect{1056, 78, 24, 25};

   // texture rect 1
   // 1056, 28
   // 1080, 53

   // texture rect 2
   // 1056, 78
   // 1080, 100

   std::array<uint8_t, 4> color = {255, 255, 255, 100};

   const auto color_it = data._tmx_object->_properties->_map.find("color");
   if (color_it != data._tmx_object->_properties->_map.end())
   {
      color = TmxTools::color(color_it->second->_value_string.value());
   }

   auto sprite = 1;
   const auto sprite_it = data._tmx_object->_properties->_map.find("sprite");
   if (sprite_it != data._tmx_object->_properties->_map.end())
   {
      sprite = sprite_it->second->_value_int.value();
   }

   if (sprite == 2)
   {
      _lamp_sprite.setTextureRect(_lamp_sprite_rect_2);
      _lamp_sprite.setOrigin(
         static_cast<float>(_lamp_sprite_rect_2.width / 2),
         static_cast<float>(_lamp_sprite_rect_2.height / 2)
      );
   }
   else
   {
      _lamp_sprite.setTextureRect(_lamp_sprite_rect_1);
      _lamp_sprite.setOrigin(
         static_cast<float>(_lamp_sprite_rect_1.width / 2),
         static_cast<float>(_lamp_sprite_rect_1.height / 2)
      );
   }

   // add raycast light
   _light = LightSystem::createLightInstance(this, {});
   _light->_color = sf::Color(color[0], color[1], color[2], color[3]);
   Level::getCurrentLevel()->getLightSystem()->_lights.push_back(_light);
}
