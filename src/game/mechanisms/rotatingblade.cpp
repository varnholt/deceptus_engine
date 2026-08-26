#include "game/mechanisms/rotatingblade.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/audio/audio.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/playerregistry.h"

#include <array>

// #define DEBUG_INTERSECTION

namespace
{
static constexpr std::array rotating_blade_properties{
   PropertyInfo{.name = "enabled", .type = "bool", .default_value = true},
   PropertyInfo{.name = "blade_acceleration", .type = "float", .default_value = 1.0f},
   PropertyInfo{.name = "blade_deceleration", .type = "float", .default_value = 0.5f},
   PropertyInfo{.name = "blade_rotation_speed", .type = "float", .default_value = 5.0f},
   PropertyInfo{.name = "movement_speed", .type = "float", .default_value = 1.0f},
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{30}},
};
static constexpr MechanismSchema rotating_blade_schema{
   .type_name = "RotatingBlade",
   .layer_name = "rotating_blades",
   .default_width = 0,
   .default_height = 0,
   .properties = rotating_blade_properties,
   .default_polyline = "0,0 48,0 96,0",
};
const auto registered_rotatingblade = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(rotating_blade_schema);

   registry.mapGroupToLayer("RotatingBlade", "rotating_blades");

   registry.registerLayerName(
      "rotating_blades",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RotatingBlade>(parent);
         mechanism->setup(data);
         mechanisms["rotating_blades"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "RotatingBlade",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<RotatingBlade>(parent);
         mechanism->setup(data);
         mechanisms["rotating_blades"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

RotatingBlade::RotatingBlade(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(RotatingBlade).name());

   setZ(30);

   _texture_map = TexturePool::getInstance().get("data/sprites/enemy_rotating_blade.png");

#ifdef DECEPTUS_VRSFML
   _sprite = std::make_unique<sf::Sprite>();
#else
   _sprite = std::make_unique<sf::Sprite>(*_texture_map);
#endif
   sfcompat::setOrigin(*_sprite, {_texture_map->getSize().x * 0.5f, _texture_map->getSize().y * 0.5f});

   _audio_update_data._range = AudioRange{600.0f, 0.0f, 100.0f, 1.0f};
   _has_audio = true;
}

RotatingBlade::~RotatingBlade()
{
   if (_sample_enabled.has_value())
   {
      Audio::getInstance().stopSample(_sample_enabled.value());
   }

   if (_sample_accelerate.has_value())
   {
      Audio::getInstance().stopSample(_sample_accelerate.value());
   }

   if (_sample_decelerate.has_value())
   {
      Audio::getInstance().stopSample(_sample_decelerate.value());
   }
}

std::string_view RotatingBlade::objectName() const
{
   return "RotatingBlade";
}

void RotatingBlade::setup(const GameDeserializeData& data)
{
   if (!data._tmx_object->_polygon && !data._tmx_object->_polyline)
   {
      Log::Error() << "the tmx object is neither a polygon or polyline";
      return;
   }

   _path = data._tmx_object->_polygon ? data._tmx_object->_polygon->_polyline : data._tmx_object->_polyline->_path;
   _path.push_back(_path.at(0));  // close path
   _path_type = data._tmx_object->_polygon ? PathType::Polygon : PathType::Polyline;

   std::transform(
      _path.begin(),
      _path.end(),
      _path.begin(),
      [data](const auto& vec) { return vec + sf::Vector2f{data._tmx_object->_x_px, data._tmx_object->_y_px}; }
   );

   _path_interpolation.addKeys(_path);

   // collision rect for lever
   _rectangle = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {64, 64}};

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         setEnabled(enabled_it->second->_value_bool.value());
      }

      const auto blade_acceleration_it = data._tmx_object->_properties->_map.find("blade_acceleration");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_acceleration = blade_acceleration_it->second->_value_float.value();
      }

      const auto blade_deceleration_it = data._tmx_object->_properties->_map.find("blade_deceleration");
      if (blade_deceleration_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_deceleration = blade_deceleration_it->second->_value_float.value();
      }

      const auto _blade_rotation_speed_it = data._tmx_object->_properties->_map.find("blade_rotation_speed");
      if (_blade_rotation_speed_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_rotation_speed = _blade_rotation_speed_it->second->_value_float.value();
      }

      const auto movement_speed_it = data._tmx_object->_properties->_map.find("movement_speed");
      if (movement_speed_it != data._tmx_object->_properties->_map.end())
      {
         _settings._movement_speed = movement_speed_it->second->_value_float.value();
      }
   }
}

void RotatingBlade::preload()
{
   Audio::getInstance().addSample("mechanism_rotating_blade_accelerate.ogg");
   Audio::getInstance().addSample("mechanism_rotating_blade_decelerate.ogg");
   Audio::getInstance().addSample("mechanism_rotating_blade_enabled.ogg");
}

void RotatingBlade::updateAudio()
{
   if (!isAudioEnabled())
   {
      // stop whatever is playing if audio is disabled
      if (_sample_enabled.has_value())
      {
         Audio::getInstance().stopSample(_sample_enabled.value());
         _sample_enabled.reset();
      }

      if (_sample_accelerate.has_value())
      {
         Audio::getInstance().stopSample(_sample_accelerate.value());
         _sample_accelerate.reset();
      }

      if (_sample_decelerate.has_value())
      {
         Audio::getInstance().stopSample(_sample_decelerate.value());
         _sample_decelerate.reset();
      }

      return;
   }

   constexpr auto eps_enabled_on = 0.5f;
   constexpr auto eps_enabled_off = 0.05f;
   constexpr auto eps_accelerate_off = 0.05f;

   // blades are accelerating until rotating at regular speed
   if (_enabled)
   {
      if (_velocity > 1.0f - eps_enabled_on)
      {
         // play regular sample
         if (!_sample_enabled.has_value())
         {
            _sample_enabled = Audio::getInstance().playSample({"mechanism_rotating_blade_enabled.ogg", 1.0f, true});
         }
         else
         {
            if (_velocity > 1.0f - eps_accelerate_off)
            {
               _sample_accelerate.reset();
            }

            Audio::getInstance().setPosition(_sample_enabled.value(), _pos);
         }
      }
      else
      {
         // play acceleration sample
         if (!_sample_accelerate.has_value())
         {
            _sample_accelerate = Audio::getInstance().playSample({"mechanism_rotating_blade_accelerate.ogg"});
         }
         else
         {
            Audio::getInstance().setPosition(_sample_accelerate.value(), _pos);
         }
      }
   }

   // blades are slowing down until they're fully stopped
   else
   {
      if (_velocity < 0.0 + eps_enabled_off)
      {
         // stop decelerate sample
         if (_sample_decelerate.has_value())
         {
            Audio::getInstance().stopSample(_sample_decelerate.value());
            _sample_decelerate.reset();
         }
      }
      else
      {
         // play deceleration sample
         if (!_sample_decelerate.has_value())
         {
            _sample_decelerate = Audio::getInstance().playSample({"mechanism_rotating_blade_decelerate.ogg"});
         }
         else
         {
            Audio::getInstance().setPosition(_sample_decelerate.value(), _pos);
         }

         // stop playing enabled sample if it's been playing before
         if (_sample_enabled.has_value())
         {
            Audio::getInstance().stopSample(_sample_enabled.value());
         }
      }
   }
}

void RotatingBlade::updateSpritePositions()
{
   // the rotation is interpolated too, otherwise the blade turns in steps while it travels smoothly
   const auto alpha = RenderInterpolation::getAlpha();
   sfcompat::setRotation(*_sprite, sf::degrees(_angle_previous + (_angle - _angle_previous) * alpha));
   sfcompat::setPosition(*_sprite, _interpolated_position.getPositionPx());
}

void RotatingBlade::update(const sf::Time& dt)
{
   if (_enabled)
   {
      _velocity = std::min<float>(1.0f, _velocity + _settings._blade_acceleration);
   }
   else
   {
      _velocity = std::max<float>(0.0f, _velocity - _settings._blade_deceleration);
   }

   // update position and rotation along path
   const auto movement_delta = dt.asSeconds() * _velocity * _settings._movement_speed;
   _path_interpolation.updateTime(movement_delta);
   _angle_previous = _angle;
   _angle += dt.asSeconds() * _velocity * _direction * _settings._blade_rotation_speed;
   _pos = _path_interpolation.computePosition(_path_interpolation.getTime());
   _interpolated_position.step(_pos.x, _pos.y);

   updateAudio();

   // kill player if he moves into the blade's radius
#ifdef DECEPTUS_VRSFML
   sf::Vector2i blade_position{static_cast<int32_t>(_sprite->position.x), static_cast<int32_t>(_sprite->position.y)};
#else
   sf::Vector2i blade_position{_sprite->getPosition()};
#endif
   const auto blade_radius = static_cast<int32_t>(_texture_map->getSize().x * 0.5f);
   if (SfmlMath::intersectCircleRect(blade_position, blade_radius, PlayerRegistry::getFirst()->getPixelRectInt()))
   {
      if (_velocity > 0.3f)
      {
         PlayerRegistry::getFirst()->damage(100);
      }
   }
}

#ifdef DECEPTUS_VRSFML
void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/, const sf::RenderStates& states)
{
   sf::RenderStates draw_states = states;
   draw_states.texture = _texture_map.get();
   target.draw(*_sprite, draw_states);

#ifdef DEBUG_INTERSECTION
   sf::Vector2i sprite_center{_sprite->position};
   const auto blade_radius = static_cast<int32_t>(_texture_map->getSize().x * 0.5f);

   b2Color color{1.0f, 1.0f, 1.0f};
   if (SfmlMath::intersectCircleRect(sprite_center, blade_radius, PlayerRegistry::getFirst()->getPlayerPixelRect()))
   {
      color = b2Color{1.0f, 0.0f, 0.0f};
   }

   DebugDraw::drawCircle(target, _sprite->position, _sprite->origin.x, color);
#endif
}
#else
void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(*_sprite);

#ifdef DEBUG_INTERSECTION
   sf::Vector2i sprite_center{_sprite->getPosition()};
   const auto blade_radius = static_cast<int32_t>(_texture_map->getSize().x * 0.5f);

   b2Color color{1.0f, 1.0f, 1.0f};
   if (SfmlMath::intersectCircleRect(sprite_center, blade_radius, PlayerRegistry::getFirst()->getPlayerPixelRect()))
   {
      color = b2Color{1.0f, 0.0f, 0.0f};
   }

   DebugDraw::drawCircle(target, _sprite->getPosition(), _sprite->getOrigin().x, color);
#endif
}
#endif

void RotatingBlade::setAudioEnabled(bool enabled)
{
   GameMechanism::setAudioEnabled(enabled);
}

void RotatingBlade::setReferenceVolume(float volume)
{
   GameMechanism::setReferenceVolume(volume);

   if (_sample_enabled.has_value())
   {
      Audio::getInstance().setVolume(_sample_enabled.value(), volume);
   }

   if (_sample_accelerate.has_value())
   {
      Audio::getInstance().setVolume(_sample_accelerate.value(), volume);
   }

   if (_sample_decelerate.has_value())
   {
      Audio::getInstance().setVolume(_sample_decelerate.value(), volume);
   }
}

std::optional<sf::FloatRect> RotatingBlade::getBoundingBoxPx()
{
   return _rectangle;
}

const sf::FloatRect& RotatingBlade::getPixelRect() const
{
   return _rectangle;
}
