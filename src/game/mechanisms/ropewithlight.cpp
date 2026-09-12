#include "ropewithlight.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/valuereader.h"
#include "game/level/levelregistry.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <array>

int32_t RopeWithLight::_lamp_instance_counter = 0;

namespace
{
constexpr auto lamp_frame_pitch_px = 24;
constexpr auto lamp_frame_duration_s = 0.12f;

/// \brief one lamp variant in the rope sprite sheet, plus the flicker frames stored next to it.
struct LampSprite
{
   sf::IntRect _first_frame_rect_px;  //!< leftmost frame of the variant in the rope sprite sheet
   int32_t _frame_count{1};           //!< flicker frames following it to the right, including itself
};

//!< lamp 2 is delivered with a single drawing only, so it burns at a constant brightness
constexpr std::array lamp_sprites{
   LampSprite{._first_frame_rect_px = sf::IntRect{{32, 0}, {16, 28}}, ._frame_count = 6},
   LampSprite{._first_frame_rect_px = sf::IntRect{{32, 48}, {20, 25}}, ._frame_count = 1},
   LampSprite{._first_frame_rect_px = sf::IntRect{{32, 96}, {16, 28}}, ._frame_count = 6},
};

static constexpr std::array rope_with_light_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
   PropertyInfo{.name = "texture", .type = "string", .default_value = default_rope_texture},
   PropertyInfo{.name = "sprite", .type = "int", .default_value = int32_t{1}},
   PropertyInfo{.name = "lamp_sprite", .type = "int", .default_value = int32_t{1}},
};
static constexpr MechanismSchema rope_with_light_schema{
   .type_name = "RopeWithLight",
   .layer_name = "ropes_with_light",
   .default_width = 24,
   .default_height = 96,
   .properties = rope_with_light_properties,
};
const auto registered_ropewithlight = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(rope_with_light_schema);

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
   draw(color, normal, {});
}

void RopeWithLight::draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   Rope::draw(color, normal, states);

   sf::RenderStates lamp_states = states;
   lamp_states.texture = _texture.get();
   color.draw(*_lamp_sprite, lamp_states);
}

void RopeWithLight::update(const sf::Time& dt)
{
   Rope::update(dt);

   if (_lamp_frame_count > 1)
   {
      _lamp_frame_elapsed_s += dt.asSeconds();
      while (_lamp_frame_elapsed_s > lamp_frame_duration_s)
      {
         _lamp_frame_elapsed_s -= lamp_frame_duration_s;
         _lamp_frame_index = (_lamp_frame_index + 1) % _lamp_frame_count;
      }
   }

   _light->_pos_m = _chain_elements.back()->GetPosition();
   _light->updateSpritePosition();

   const auto c1_pos_m = _chain_elements[_chain_elements.size() - 2]->GetPosition();
   const auto c2_pos_m = _chain_elements[_chain_elements.size() - 1]->GetPosition();
   const auto c_m = (c1_pos_m - c2_pos_m);

   const auto angle_rad = static_cast<float>(atan2(c_m.y, c_m.x));

   _lamp_position_px_previous = _lamp_position_px_current;
   _lamp_position_px_current = sf::Vector2f{_light->_pos_m.x * PPM, _light->_pos_m.y * PPM};

   _lamp_rotation_deg_previous = _lamp_rotation_deg_current;
   _lamp_rotation_deg_current = 90.0f + FACTOR_RAD_TO_DEG * angle_rad;
}

void RopeWithLight::updateSpritePositions()
{
   Rope::updateSpritePositions();

   const auto position_px = RenderInterpolation::positionPx(_lamp_position_px_previous, _lamp_position_px_current);

   // the rotation is interpolated too, otherwise the lamp swings in steps while it travels smoothly.
   // it is not rounded: an angle has no pixel grid to sit on
   const auto alpha = RenderInterpolation::getAlpha();
   const auto rotation_deg = _lamp_rotation_deg_previous + (_lamp_rotation_deg_current - _lamp_rotation_deg_previous) * alpha;

   auto frame_rect_px = _lamp_frame_rect_px;
   frame_rect_px.position.x += _lamp_frame_index * lamp_frame_pitch_px;

#ifdef DECEPTUS_VRSFML
   _lamp_sprite->textureRect = frame_rect_px;
   _lamp_sprite->rotation = sf::degrees(rotation_deg);
   _lamp_sprite->position = position_px;
#else
   _lamp_sprite->setTextureRect(frame_rect_px);
   _lamp_sprite->setRotation(sf::degrees(rotation_deg));
   _lamp_sprite->setPosition(position_px);
#endif
}

void RopeWithLight::setup(const GameDeserializeData& data)
{
   Rope::setup(data);

   // set up texture
#ifdef DECEPTUS_VRSFML
   _lamp_sprite = std::make_unique<sf::Sprite>();
#else
   _lamp_sprite = std::make_unique<sf::Sprite>(*_texture);
#endif

   const auto& map = data._tmx_object->_properties->_map;

   const auto lamp_sprite_index =
      std::clamp(ValueReader::readValue<int32_t>("lamp_sprite", map).value_or(1) - 1, 0, static_cast<int32_t>(lamp_sprites.size()) - 1);
   _lamp_frame_rect_px = lamp_sprites[lamp_sprite_index]._first_frame_rect_px;
   _lamp_frame_count = lamp_sprites[lamp_sprite_index]._frame_count;

   // start each lamp on a different frame so a row of them does not flicker in lockstep
   _lamp_instance_counter++;
   _lamp_frame_index = _lamp_instance_counter % _lamp_frame_count;

#ifdef DECEPTUS_VRSFML
   _lamp_sprite->textureRect = _lamp_frame_rect_px;
   _lamp_sprite->origin = {static_cast<float>(_lamp_frame_rect_px.size.x / 2), static_cast<float>(_lamp_frame_rect_px.size.y / 2)};
#else
   _lamp_sprite->setTextureRect(_lamp_frame_rect_px);
   _lamp_sprite->setOrigin({static_cast<float>(_lamp_frame_rect_px.size.x / 2), static_cast<float>(_lamp_frame_rect_px.size.y / 2)});
#endif

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

#ifdef DECEPTUS_VRSFML
   _light->_sprite->scale = {
      static_cast<float>(_light->_width_px) / _light->_texture->getSize().x,
      static_cast<float>(_light->_height_px) / _light->_texture->getSize().y
   };
#else
   _light->_sprite->setScale(
      {static_cast<float>(_light->_width_px) / _light->_texture->getSize().x,
       static_cast<float>(_light->_height_px) / _light->_texture->getSize().y}
   );
#endif

   for (auto* body : _chain_elements)
   {
      _light->_excluded_bodies.insert(body);
   }
   LevelRegistry::getCurrent()->getLightSystem()->_lights.push_back(_light);
}
