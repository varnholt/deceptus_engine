#include "ropewithlight.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/valuereader.h"
#include "game/level/levelregistry.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
const auto registered_ropewithlight = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("RopeWithLight", "ropes_with_light");

   registry.registerLayerName(
      "ropes_with_light",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RopeWithLight>(parent);
         mechanism->setup(data);
         mechanisms["ropes"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "RopeWithLight",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RopeWithLight>(parent);
         mechanism->setup(data);
         mechanisms["ropes"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

RopeWithLight::RopeWithLight(GameNode* parent) : Rope(parent)
{
   setClassName(typeid(RopeWithLight).name());
}

std::string_view RopeWithLight::objectName() const
{
   return "RopeWithLight";
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
   _lamp_sprite->setPosition({_light->_pos_m.x * PPM, _light->_pos_m.y * PPM});
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

   const auto& map = data._tmx_object->_properties->_map;

   auto sprite_index = std::clamp(ValueReader::readValue<int32_t>("sprite", map).value_or(1) - 1, 0, 3);
   _lamp_sprite->setTextureRect(_lamp_sprite_rects[sprite_index]);
   _lamp_sprite->setOrigin(
      {static_cast<float>(_lamp_sprite_rects[sprite_index].size.x / 2), static_cast<float>(_lamp_sprite_rects[sprite_index].size.y / 2)}
   );

   // add raycast light; exclude all chain bodies from shadow casting — they are tiny
   // physics proxies that produce degenerate or distracting shadow quads.
   _light = LightSystem::createLightInstance(this, data);

   if (const auto width = ValueReader::readValue<int32_t>("width_px", map))
   {
      _light->_width_px = width.value();
   }

   if (const auto height = ValueReader::readValue<int32_t>("height_px", map))
   {
      _light->_height_px = height.value();
   }

   _light->_sprite->setScale(
      {static_cast<float>(_light->_width_px) / _light->_texture->getSize().x,
       static_cast<float>(_light->_height_px) / _light->_texture->getSize().y}
   );

   for (auto* body : _chain_elements)
   {
      _light->_excluded_bodies.insert(body);
   }
   LevelRegistry::getCurrent()->getLightSystem()->_lights.push_back(_light);
}
