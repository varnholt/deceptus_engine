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
      _config.row = ValueReader::readValue<int32_t>("row", map).value_or(_config.row);
      _config.max_hits = ValueReader::readValue<int32_t>("hits", map).value_or(_config.max_hits);

      _config.hit_sound = ValueReader::readValue<std::string>("hit_sound", map).value_or(_config.hit_sound);
      _config.destroy_sound = ValueReader::readValue<std::string>("destroy_sound", map).value_or(_config.destroy_sound);
      _config.texture_path = ValueReader::readValue<std::string>("texture", map).value_or(_config.texture_path);

      _config.z_index = ValueReader::readValue<int32_t>("z", map).value_or(_config.z_index);
   }

   _state.hits_left = _config.max_hits;

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
}

std::string_view DestructibleBlockingRect::objectName() const
{
   return "DestructibleBlockingRect";
}

std::optional<sf::FloatRect> DestructibleBlockingRect::getBoundingBoxPx()
{
   return _state.dead ? std::nullopt : std::optional<sf::FloatRect>(_rect_px);
}

void DestructibleBlockingRect::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{

   color.draw(*_sprite);

   // if (_normal_map)
   // {
   //    auto old_texture = _sprite->getTexture();
   //    _sprite->setTexture(*_normal_map);
   //    normal.draw(*_sprite);
   //    _sprite->setTexture(old_texture);
   // }
}

void DestructibleBlockingRect::update(const sf::Time& /*dt*/)
{
   // // Advance the animation frame up to the maximum number of configured frames
   // ++_state.current_frame;
   // if (_state.current_frame >= _config.frame_count)
   // {
   //    _state.current_frame = _config.frame_count - 1;
   // }

   // // Update sprite rect accordingly
   // if (_sprite)
   // {
   //    _sprite->setTextureRect(
   //       sf::IntRect{
   //          {_state.current_frame * _config.frame_width, _config.row * _config.frame_height}, {_config.frame_width, _config.frame_height}
   //       }
   //    );
   // }
}

void DestructibleBlockingRect::onHit(int32_t damage)
{
   if (_state.dead)
   {
      return;
   }

   _state.hits_left -= damage;

   if (!_config.hit_sound.empty())
   {
      Audio::getInstance().playSample({_config.hit_sound});
   }

   if (_state.hits_left <= 0)
   {
      destroy();
   }
}

void DestructibleBlockingRect::setupBody(const GameDeserializeData& data)
{
   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;

   // the physical blocking area is fixed at 48x96 pixels; convert to Box2D meters.
   // box2D's SetAsBox method expects half-extents. compute half widths and
   // heights in metres and keep the full extents for the bounding box.
   constexpr float blocking_width_px = 48.0f;
   constexpr float blocking_height_px = 96.0f;
   const float half_width_m = 0.5f * blocking_width_px * MPP;
   const float half_height_m = 0.5f * blocking_height_px * MPP;

   // Set up bounding box for chunking/culling
   _rect_px = sf::FloatRect{{static_cast<float>(x_px), static_cast<float>(y_px)}, {blocking_width_px, blocking_height_px}};
   addChunks(_rect_px);

   // Create Box2D body; position at the TMX object's coordinate, scaled to meters
   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = MPP * b2Vec2{static_cast<float>(x_px), static_cast<float>(y_px)};
   _body = data._world->CreateBody(&body_def);

   // Define a rectangle shape; Box2D boxes are defined by half-extents and an offset.
   // Provide the half-width and half-height computed above and shift the shape by
   // the same amount so the rectangle's bottom-left corner matches the TMX
   // object's position.
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

   // // attempt to load a normal map by inserting "_normals" before the file
   // // extension.  Skip if the file does not exist.
   // const auto stem = resolved_texture_path.stem().string();
   // const auto ext = resolved_texture_path.extension().string();
   // std::filesystem::path normal_map_path = resolved_texture_path.parent_path() / (stem + "_normals" + ext);
   // if (std::filesystem::exists(normal_map_path))
   // {
   //    _normal_map = TexturePool::getInstance().get(normal_map_path);
   // }
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
