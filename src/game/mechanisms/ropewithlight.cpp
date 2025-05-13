#include "ropewithlight.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "game/io/valuereader.h"
#include "game/level/level.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
const auto registered_ropewithlight = []
{
   GameMechanismDeserializerRegistry::instance().registerLayer(
      "ropes_with_light",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RopeWithLight>(parent);
         mechanism->setup(data);
         mechanisms["ropes_with_light"]->push_back(mechanism);
      }
   );
   GameMechanismDeserializerRegistry::instance().registerTemplateType(
      "RopeWithLight",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RopeWithLight>(parent);
         mechanism->setup(data);
         mechanisms["ropes_with_light"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

RopeWithLight::RopeWithLight(GameNode* parent) : Rope(parent)
{
   setClassName(typeid(RopeWithLight).name());
}

void RopeWithLight::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   Rope::draw(color, normal);

   color.draw(*_lamp_sprite);
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

   _lamp_sprite->setRotation(sf::degrees(90.0f + FACTOR_RAD_TO_DEG * angle_rad));
   _lamp_sprite->setPosition(
      {_light->_pos_m.x * PPM,  // - _lamp_sprite_rect.size.x / 2,
       _light->_pos_m.y * PPM}  // - _lamp_sprite_rect.size.y / 2
   );
}

void RopeWithLight::setup(const GameDeserializeData& data)
{
   Rope::setup(data);

   // set up texture
   _lamp_sprite = std::make_unique<sf::Sprite>(*_texture);

   // cut off 1st 4 pixels of the texture rect since there's some rope pixels in the spriteset
   _lamp_sprite_rects = {
      sf::IntRect{{1056, 28}, {24, 28}},
      sf::IntRect{{1056, 78}, {24, 25}},
      sf::IntRect{{1056, 131}, {24, 30}},
   };

   std::array<uint8_t, 4> color = {255, 255, 255, 100};
   const auto map = data._tmx_object->_properties->_map;
   const auto color_it = map.find("color");
   if (color_it != map.end())
   {
      color = TmxTools::color(color_it->second->_value_string.value());
   }

   auto sprite_index = std::clamp(ValueReader::readValue<int32_t>("sprite", map).value_or(1) - 1, 0, 3);
   _lamp_sprite->setTextureRect(_lamp_sprite_rects[sprite_index]);
   _lamp_sprite->setOrigin(
      {static_cast<float>(_lamp_sprite_rects[sprite_index].size.x / 2), static_cast<float>(_lamp_sprite_rects[sprite_index].size.y / 2)}
   );

   // add raycast light
   _light = LightSystem::createLightInstance(this, {});
   _light->_color = sf::Color(color[0], color[1], color[2], color[3]);
   Level::getCurrentLevel()->getLightSystem()->_lights.push_back(_light);
}
