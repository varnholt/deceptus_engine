#include "staticlight.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/globalclock.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <array>
#include <filesystem>

namespace
{
static constexpr std::string_view default_static_light_color = "#ffffffff";
static constexpr std::string_view default_static_light_texture = "smooth.png";
static constexpr float default_static_light_flicker_intensity = 0.0f;
static constexpr float default_static_light_flicker_speed = 0.0f;
static constexpr float default_static_light_flicker_alpha_amount = 1.0f;
static constexpr int32_t default_static_light_z = 20;
static constexpr std::array static_light_properties{
   PropertyInfo{.name = "color", .type = "string", .default_value = default_static_light_color},
   PropertyInfo{.name = "texture", .type = "string", .default_value = default_static_light_texture},
   PropertyInfo{.name = "flicker_intensity", .type = "float", .default_value = default_static_light_flicker_intensity},
   PropertyInfo{.name = "flicker_speed", .type = "float", .default_value = default_static_light_flicker_speed},
   PropertyInfo{.name = "flicker_alpha_amount", .type = "float", .default_value = default_static_light_flicker_alpha_amount},
   PropertyInfo{.name = "z", .type = "int", .default_value = default_static_light_z},
};
static constexpr MechanismSchema static_light_schema{
   .type_name = "StaticLight",
   .layer_name = "static_lights",
   .default_width = 96,
   .default_height = 96,
   .properties = static_light_properties,
};
const auto registered_staticlight = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(static_light_schema);
   registry.markAsNonVisual("static_lights");
   registry.mapGroupToLayer("StaticLight", "static_lights");

   registry.registerLayerName(
      "static_lights",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<StaticLight>(parent);
         mechanism->deserialize(data);
         mechanisms["static_lights"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "StaticLight",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<StaticLight>(parent);
         mechanism->deserialize(data);
         mechanisms["static_lights"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
auto instance_count = 0;
}

StaticLight::StaticLight(GameNode* parent) : GameNode(parent), _instance_number(instance_count++)
{
   setClassName(typeid(StaticLight).name());
}

std::string_view StaticLight::objectName() const
{
   return "StaticLight";
}

#ifdef __EMSCRIPTEN__
void StaticLight::draw(sf::RenderTarget& target, sf::RenderTarget& color)
{
   draw(target, color, {});
}

void StaticLight::draw(sf::RenderTarget& target, sf::RenderTarget& /*color*/, const sf::RenderStates& states)
{
   auto lumen = fbm::mix(_color.a, _flicker_amount * 255.0f, 1.0f - _flicker_alpha_amount);

   sf::Color light_color{_color.r, _color.g, _color.b, static_cast<uint8_t>(lumen)};

   _sprite->color = light_color;

   sf::RenderStates draw_states = states;
   draw_states.blendMode = _blend_mode;
   draw_states.texture = _texture.get();
   target.draw(*_sprite, draw_states);
}
#else
void StaticLight::draw(sf::RenderTarget& target, sf::RenderTarget& /*color*/)
{
   auto lumen = fbm::mix(_color.a, _flicker_amount * 255.0f, 1.0f - _flicker_alpha_amount);

   sf::Color color{_color.r, _color.g, _color.b, static_cast<uint8_t>(lumen)};

   _sprite->setColor(color);
   target.draw(*_sprite, _blend_mode);
}
#endif

void StaticLight::update(const sf::Time& /*time*/)
{
   const auto time = GlobalClock::getInstance().getElapsedTime();

   if (_flicker_intensity > 0.0001f)
   {
      _flicker_amount = _flicker_intensity * fbm::fbm(fbm::vec2{
                                                _instance_number + time.asSeconds() * _flicker_speed,
                                                _time_offset + _instance_number / static_cast<float>(instance_count)
                                             });
   }
}

std::optional<sf::FloatRect> StaticLight::getBoundingBoxPx()
{
   return std::nullopt;
}

void StaticLight::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";
   auto flicker_intensity = 0.0f;
   auto flicker_alpha_amount = 1.0f;
   auto flicker_speed = 0.0f;

   if (data._tmx_object->_properties)
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

      it = data._tmx_object->_properties->_map.find("z");
      if (it != data._tmx_object->_properties->_map.end())
      {
         _z_index = it->second->_value_int.value();
      }
   }

   _color.r = rgba[0];
   _color.g = rgba[1];
   _color.b = rgba[2];
   _color.a = rgba[3];
   _flicker_intensity = flicker_intensity;
   _flicker_alpha_amount = flicker_alpha_amount;
   _flicker_speed = flicker_speed;
   _texture = TexturePool::getInstance().get(texture);
#ifdef __EMSCRIPTEN__
   _sprite = std::make_unique<sf::Sprite>();
   _sprite->textureRect =
      sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(_texture->getSize().x), static_cast<float>(_texture->getSize().y)}};
   _sprite->color = _color;
   _sprite->position = {data._tmx_object->_x_px, data._tmx_object->_y_px};
#else
   _sprite = std::make_unique<sf::Sprite>(*_texture);
   _sprite->setColor(_color);
   _sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});
#endif

   _rect = sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};
   addChunks(_rect);

   auto scale_x_px = data._tmx_object->_width_px / _texture->getSize().x;
   auto scale_y_px = data._tmx_object->_height_px / _texture->getSize().y;
#ifdef __EMSCRIPTEN__
   _sprite->scale = {scale_x_px, scale_y_px};
#else
   _sprite->scale({scale_x_px, scale_y_px});
#endif

   // init each light with a different time offset
   // probably passing the position itself to FBM would be enough
   std::srand(static_cast<uint32_t>(data._tmx_object->_x_px * data._tmx_object->_y_px));
   _time_offset = (std::rand() % 100) * 0.01f;
}
