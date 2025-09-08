#include "DestructibleBlockingRect.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

#include "game/audio/audio.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

#include <filesystem>
#include <iostream>

namespace
{
const auto registered_destructible_blocking_rect = []()
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("DestructibleBlockingRect", "destructible_blocking_rects");
   registry.registerLayerName(
      "destructible_blocking_rects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<DestructibleBlockingRect>(parent, data);
         mechanisms["destructible_blocking_rects"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "DestructibleBlockingRect",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<DestructibleBlockingRect>(parent, data);
         mechanisms["destructible_blocking_rects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

DestructibleBlockingRect::DestructibleBlockingRect(GameNode* parent, const GameDeserializeData& data) : FixtureNode(parent), GameMechanism()
{
   setClassName(typeid(DestructibleBlockingRect).name());
   setType(ObjectTypeObstacle);
   setObjectId(data._tmx_object->_name);

   // read custom properties
   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;

      _config.frame_width = ValueReader::readValue<int32_t>("frame_width", map).value_or(_config.frame_width);
      _config.frame_height = ValueReader::readValue<int32_t>("frame_height", map).value_or(_config.frame_height);
      _config.frame_count = ValueReader::readValue<int32_t>("frame_count", map).value_or(_config.frame_count);
      _config.alignment = ValueReader::readValue<bool>("right_aligned", map).value_or(false) ? Alignment::Right : _config.alignment;
      _config.max_damage = ValueReader::readValue<int32_t>("max_damage", map).value_or(_config.max_damage);

      _config.hit_sound = ValueReader::readValue<std::string>("hit_sound", map).value_or(_config.hit_sound);
      _config.destroy_sound = ValueReader::readValue<std::string>("destroy_sound", map).value_or(_config.destroy_sound);
      _config.texture_path = ValueReader::readValue<std::string>("texture", map).value_or(_config.texture_path);
      _config.animation_speed = ValueReader::readValue<float>("animation_speed", map).value_or(_config.animation_speed);

      _config.z_index = ValueReader::readValue<int32_t>("z", map).value_or(_config.z_index);
   }

   _config.row = (_config.alignment == Alignment::Right) ? 1 : 0;

   Audio::getInstance().addSample(_config.hit_sound);
   Audio::getInstance().addSample(_config.destroy_sound);

   _state.damage_left = _config.max_damage;

   setupBody(data);
   setupSprite(data);

   if (!_config.hit_sound.empty())
   {
      Audio::getInstance().addSample(_config.hit_sound);
   }

   if (!_config.destroy_sound.empty())
   {
      Audio::getInstance().addSample(_config.destroy_sound);
   }

   setZ(_config.z_index);

   if (!_flash_shader.loadFromFile("data/shaders/flash.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading flash shader";
   }

   _flash_shader.setUniform("texture", sf::Shader::CurrentTexture);
   _flash_shader.setUniform("flash", _hit_flash);
}

std::string_view DestructibleBlockingRect::objectName() const
{
   return "DestructibleBlockingRect";
}

std::optional<sf::FloatRect> DestructibleBlockingRect::getBoundingBoxPx()
{
   return _state.dead ? std::nullopt : std::optional<sf::FloatRect>(_rect_px);
}

const std::vector<Hitbox>& DestructibleBlockingRect::getHitboxes()
{
   return _hitboxes;
}

bool DestructibleBlockingRect::isDestructible() const
{
   return true;
}

void DestructibleBlockingRect::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   color.draw(*_sprite, &_flash_shader);
}

void DestructibleBlockingRect::update(const sf::Time& dt)
{
   if (_hit_time.has_value())
   {
      std::chrono::duration<float> hit_duration_s = (std::chrono::high_resolution_clock::now() - _hit_time.value());
      constexpr auto hit_duration_max_s = 0.3f;
      if (hit_duration_s.count() > hit_duration_max_s)
      {
         _hit_time.reset();
         _hit_flash = 0.0f;
      }
      else
      {
         _hit_flash = 1.0f - (hit_duration_s.count() / hit_duration_max_s);
      }

      _flash_shader.setUniform("flash", _hit_flash);
   }

   if (_state.dead)
   {
      _state.current_frame += dt.asSeconds() * _config.animation_speed;

      if (_state.current_frame >= _config.frame_count)
      {
         _state.current_frame = _config.frame_count - 1;
      }

      _sprite->setTextureRect(
         sf::IntRect{
            {static_cast<int32_t>(_state.current_frame) * _config.frame_width, _config.row * _config.frame_height},
            {_config.frame_width, _config.frame_height}
         }
      );
   }
}

void DestructibleBlockingRect::hit(int32_t damage)
{
   if (_state.dead)
   {
      return;
   }

   _hit_time = std::chrono::high_resolution_clock::now();
   _state.damage_left -= damage;

   if (!_config.hit_sound.empty())
   {
      Audio::getInstance().playSample({_config.hit_sound});
   }

   if (_state.damage_left <= 0)
   {
      destroy();
   }
}

void DestructibleBlockingRect::setupBody(const GameDeserializeData& data)
{
   constexpr auto blocking_width_px = 48.0f;
   constexpr auto blocking_height_px = 96.0f;
   constexpr auto half_width_m = 0.5f * blocking_width_px * MPP;
   constexpr auto half_height_m = 0.5f * blocking_height_px * MPP;

   const auto alignment_offset_px = (_config.alignment == Alignment::Right ? (_config.frame_width - blocking_width_px) : 0);
   const auto x_px = data._tmx_object->_x_px + alignment_offset_px;
   const auto y_px = data._tmx_object->_y_px;

   _rect_px = sf::FloatRect{{static_cast<float>(x_px), static_cast<float>(y_px)}, {blocking_width_px, blocking_height_px}};
   addChunks(_rect_px);
   _hitboxes.push_back({_rect_px, {}});

   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = MPP * b2Vec2{static_cast<float>(x_px), static_cast<float>(y_px)};
   _body = data._world->CreateBody(&body_def);

   _shape.SetAsBox(half_width_m, half_height_m, b2Vec2(half_width_m, half_height_m), 0.0f);
   b2FixtureDef fixture_def;
   fixture_def.shape = &_shape;
   fixture_def.density = 1.0f;
   fixture_def.friction = 0.0f;
   fixture_def.restitution = 0.0f;
   auto* fixture = _body->CreateFixture(&fixture_def);

   // set user data so we can identify this object during collisions
   fixture->SetUserData(static_cast<void*>(this));
}

void DestructibleBlockingRect::setupSprite(const GameDeserializeData& data)
{
   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;

   _texture = TexturePool::getInstance().get(_config.texture_path);

   _sprite = std::make_unique<sf::Sprite>(*_texture);
   _sprite->setPosition(sf::Vector2f{static_cast<float>(x_px), static_cast<float>(y_px)});
   _sprite->setTextureRect(sf::IntRect{{0, _config.row * _config.frame_height}, {_config.frame_width, _config.frame_height}});
}

void DestructibleBlockingRect::destroy()
{
   if (_state.dead)
   {
      return;
   }

   _state.dead = true;

   if (!_config.destroy_sound.empty())
   {
      Audio::getInstance().playSample({_config.destroy_sound});
   }

   if (_body)
   {
      _body->SetEnabled(false);
   }

   setEnabled(false);
}

b2Body* DestructibleBlockingRect::getBody() const
{
   return _body;
}
