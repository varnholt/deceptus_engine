#include "game/mechanisms/skillgate.h"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_map>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/state/savestate.h"

namespace
{
static constexpr bool default_skill_gate_inverted = false;
static constexpr float default_skill_gate_fade_speed = 2.0f;
static constexpr auto default_skill_gate_skill = std::string_view{"double_jump"};

static constexpr std::array skill_gate_properties{
   PropertyInfo{.name = "skill", .type = "string", .default_value = default_skill_gate_skill, .required = true},
   PropertyInfo{.name = "inverted", .type = "bool", .default_value = default_skill_gate_inverted},
   PropertyInfo{.name = "fade_speed", .type = "float", .default_value = default_skill_gate_fade_speed},
   PropertyInfo{.name = "enabled", .type = "bool", .default_value = true},
   PropertyInfo{.name = "texture", .type = "string", .default_value = std::string_view{""}},
   PropertyInfo{.name = "normal", .type = "string", .default_value = std::string_view{""}},
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
};
static constexpr MechanismSchema skill_gate_schema{
   .type_name = "SkillGate",
   .layer_name = "skill_gates",
   .default_width = 48,
   .default_height = 96,
   .properties = skill_gate_properties,
};
const auto registered_skillgate = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(skill_gate_schema);

   registry.mapGroupToLayer("SkillGate", "skill_gates");

   registry.registerLayerName(
      "skill_gates",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SkillGate>(parent);
         mechanism->setup(data);
         mechanisms["skill_gates"]->push_back(mechanism);
      }
   );

   registry.registerObjectGroup(
      "SkillGate",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SkillGate>(parent);
         mechanism->setup(data);
         mechanisms["skill_gates"]->push_back(mechanism);
      }
   );
   return true;
}();

/// \brief resolves a tmx skill property to a skill flag.
/// \param skill_name lower-case skill identifier as written in the tmx property.
/// \return matching skill flag, or std::nullopt when the name is not known.
std::optional<Skill::SkillType> skillFromName(const std::string& skill_name)
{
   static const std::unordered_map<std::string, Skill::SkillType> skill_names{
      {"wall_climb", Skill::SkillType::WallClimb},
      {"dash", Skill::SkillType::Dash},
      {"invulnerable", Skill::SkillType::Invulnerable},
      {"wall_slide", Skill::SkillType::WallSlide},
      {"wall_jump", Skill::SkillType::WallJump},
      {"double_jump", Skill::SkillType::DoubleJump},
      {"crouch", Skill::SkillType::Crouch},
      {"swim", Skill::SkillType::Swim},
   };

   const auto skill_name_it = skill_names.find(skill_name);
   if (skill_name_it == skill_names.end())
   {
      return std::nullopt;
   }

   return skill_name_it->second;
}

}  // namespace

SkillGate::SkillGate(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(SkillGate).name());
}

std::string_view SkillGate::objectName() const
{
   return "SkillGate";
}

void SkillGate::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _rectangle = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   if (data._tmx_object->_properties)
   {
      const auto& properties = data._tmx_object->_properties->_map;

      const auto skill_name = ValueReader::readValue<std::string>("skill", properties);
      if (!skill_name.has_value())
      {
         Log::Warning() << "skill gate '" << getObjectId() << "' has no skill property, it will stay closed";
      }
      else
      {
         _required_skill = skillFromName(skill_name.value());
         if (!_required_skill.has_value())
         {
            Log::Warning() << "skill gate '" << getObjectId() << "' has unknown skill '" << skill_name.value() << "', it will stay closed";
         }
      }

      _inverted = ValueReader::readValue<bool>("inverted", properties).value_or(default_skill_gate_inverted);
      _fade_speed = ValueReader::readValue<float>("fade_speed", properties).value_or(default_skill_gate_fade_speed);
      _enabled = ValueReader::readValue<bool>("enabled", properties).value_or(true);
      setZ(ValueReader::readValue<int32_t>("z", properties).value_or(getZ()));

      const auto texture = ValueReader::readValue<std::string>("texture", properties);
      if (texture.has_value())
      {
         _texture_map = TexturePool::getInstance().get(texture.value());
#ifdef __EMSCRIPTEN__
         _sprite = std::make_unique<sf::Sprite>();
#else
         _sprite = std::make_unique<sf::Sprite>(*_texture_map);
#endif
         sfcompat::setPosition(*_sprite, {data._tmx_object->_x_px, data._tmx_object->_y_px});
      }

      const auto normal = ValueReader::readValue<std::string>("normal", properties);
      if (normal.has_value())
      {
         _normal_map = TexturePool::getInstance().get(normal.value());
      }
   }

   // create body
   b2BodyDef body_definition;
   body_definition.type = b2_staticBody;
   body_definition.position = b2Vec2(data._tmx_object->_x_px * MPP, data._tmx_object->_y_px * MPP);

   _body = data._world->CreateBody(&body_definition);

   const auto half_physics_width = data._tmx_object->_width_px * MPP * 0.5f;
   const auto half_physics_height = data._tmx_object->_height_px * MPP * 0.5f;

   _shape_bounds.SetAsBox(half_physics_width, half_physics_height, b2Vec2(half_physics_width, half_physics_height), 0.0f);

   b2FixtureDef boundary_fixture_definition;
   boundary_fixture_definition.shape = &_shape_bounds;
   boundary_fixture_definition.density = 1.0f;

   _body->CreateFixture(&boundary_fixture_definition);

   // snap to the initial state so gates the player already unlocked are not visible on level load
   const auto blocking = isBlocking();
   _alpha = blocking ? 1.0f : 0.0f;
   applyBlockingState(blocking);

   addChunks(_rectangle);
}

bool SkillGate::isPassable() const
{
   if (!_required_skill.has_value())
   {
      return false;
   }

   const auto skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
   const auto has_skill = (skills & static_cast<int32_t>(_required_skill.value())) != 0;

   return _inverted ? !has_skill : has_skill;
}

bool SkillGate::isBlocking() const
{
   return isEnabled() && !isPassable();
}

void SkillGate::applyBlockingState(bool blocking)
{
   if (_body->IsEnabled() != blocking)
   {
      _body->SetEnabled(blocking);
   }
}

void SkillGate::update(const sf::Time& dt)
{
   const auto blocking = isBlocking();

   // the collider follows the skill condition immediately while the sprite fades, so acquiring a skill never traps
   // the player inside a gate that is still dissolving
   applyBlockingState(blocking);

   const auto target_alpha = blocking ? 1.0f : 0.0f;
   const auto alpha_step = _fade_speed * dt.asSeconds();

   if (_alpha < target_alpha)
   {
      _alpha = std::min(_alpha + alpha_step, target_alpha);
   }
   else if (_alpha > target_alpha)
   {
      _alpha = std::max(_alpha - alpha_step, target_alpha);
   }
}

const sf::FloatRect& SkillGate::getPixelRect() const
{
   return _rectangle;
}

void SkillGate::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void SkillGate::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   // nothing to paint
   if (_sprite == nullptr)
   {
      return;
   }

   if (_alpha <= 0.0f)
   {
      return;
   }

   sfcompat::setColor(*_sprite, sf::Color(255, 255, 255, static_cast<uint8_t>(_alpha * 255.0f)));

#ifdef __EMSCRIPTEN__
   sf::RenderStates color_states = states;
   color_states.texture = _texture_map.get();
   target.draw(*_sprite, color_states);

   if (_normal_map)
   {
      sf::RenderStates normal_states = states;
      normal_states.texture = _normal_map.get();
      normal.draw(*_sprite, normal_states);
   }
#else
   target.draw(*_sprite, states);

   if (_normal_map)
   {
      _sprite->setTexture(*_normal_map);
      normal.draw(*_sprite, states);
      _sprite->setTexture(*_texture_map);
   }
#endif
}

std::optional<sf::FloatRect> SkillGate::getBoundingBoxPx()
{
   return _rectangle;
}
