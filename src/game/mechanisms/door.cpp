#include "door.h"

// game
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "game/animation/animationpool.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/level/fixturenode.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "game/state/savestate.h"

#include <iostream>

namespace
{
constexpr auto door_height_tl = 4;
}

Door::Door(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Door).name());
}

Door::~Door()
{
   // destructor for debugging purposes only
   // std::cout << "door destroyed" << std::endl;
}

void Door::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   if (_animation_open && !_animation_open->_paused)
   {
      _animation_open->draw(color);
   }
   else if (_animation_close && !_animation_close->_paused)
   {
      _animation_close->draw(color);
   }
   else if (_state == State::Closed)
   {
      color.draw(*_sprite);
   }

   if (_player_at_door)
   {
      if (_animation_key)
      {
         _animation_key->draw(color);
      }
   }

   if (_version == Version::Version1)
   {
      color.draw(_door_quad, _texture.get());
   }
}

void Door::updateBars(const sf::Time& dt)
{
   const auto left = 0.0f;
   const auto right = left + 3.0f * PIXELS_PER_TILE;
   const auto top = 3.0f * PIXELS_PER_TILE - _bar_offset;
   const auto bottom = top + 3.0f * PIXELS_PER_TILE;

   _door_quad[0].texCoords = sf::Vector2f(left, top);
   _door_quad[1].texCoords = sf::Vector2f(left, bottom);
   _door_quad[2].texCoords = sf::Vector2f(right, bottom);

   _door_quad[3].texCoords = sf::Vector2f(left, top);
   _door_quad[4].texCoords = sf::Vector2f(right, bottom);
   _door_quad[5].texCoords = sf::Vector2f(right, top);

   constexpr auto open_speed = 50.0f;
   constexpr auto close_speed = 200.0f;

   switch (_state)
   {
      case State::Opening:
      {
         _bar_offset -= open_speed * dt.asSeconds();
         if (fabs(_bar_offset) >= PIXELS_PER_TILE * door_height_tl)
         {
            _state = State::Open;
            _bar_offset = static_cast<float_t>(-PIXELS_PER_TILE * door_height_tl);
         }
         break;
      }
      case State::Closing:
      {
         _bar_offset += close_speed * dt.asSeconds();
         if (_bar_offset >= 0.0f)
         {
            _state = State::Closed;
            _bar_offset = 0.0f;
         }
         break;
      }
      case State::Open:
      case State::Closed:
      {
         break;
      }
   }

   updateTransform();
}

void Door::update(const sf::Time& dt)
{
   if (_player_at_door)
   {
      if (_animation_key)
      {
         _animation_key->update(dt);
      }
   }

   if (_animation_open && !_animation_open->_paused)
   {
      _animation_open->update(dt);
   }

   if (_animation_close && !_animation_close->_paused)
   {
      _animation_close->update(dt);
   }

   // disable body when animation is done
   if (_state == State::Opening)
   {
      if (_animation_open)
      {
         if (_animation_open->_paused)
         {
            _state = State::Open;
            _body->SetEnabled(false);
         }
      }
   }

   // enable body when animation is done
   else if (_state == State::Closing)
   {
      if (_animation_close)
      {
         if (_animation_open->_paused)
         {
            _state = State::Closed;
            _body->SetEnabled(true);
         }
      }
   }

   if (_version == Version::Version1)
   {
      updateBars(dt);
   }

   if (Player::getCurrent()->getControls()->isButtonBPressed() && checkPlayerAtDoor())
   {
      // block spamming
      using namespace std::chrono_literals;
      const auto now = std::chrono::high_resolution_clock::now();
      if (_last_toggle_time.has_value() && (now - _last_toggle_time.value()) < 1s)
      {
         return;
      }

      if (_required_item.has_value() && !SaveState::getPlayerInfo()._inventory.hasInventoryItem(*_required_item))
      {
         Log::Info() << "player doesn't have key: " << *_required_item;
         return;
      }

      toggle();

      _last_toggle_time = now;
   }
}

void Door::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);

   if (enabled)
   {
      open();
   }
   else
   {
      close();
   }
}

std::optional<sf::FloatRect> Door::getBoundingBoxPx()
{
   return _pixel_rect;
}

void Door::updateTransform()
{
   // todo: offset should be computed from rectangle dimension
   const auto _offset_x_m = PIXELS_PER_TILE * MPP;
   const auto _offset_y_m = PIXELS_PER_TILE * MPP;

   const auto x = _offset_x_m + _tile_position_tl.x * PIXELS_PER_TILE / PPM;
   const auto y = _offset_y_m + (_bar_offset + _tile_position_tl.y * PIXELS_PER_TILE) / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

void Door::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;
   const auto size_x = (PIXELS_PER_TILE / PPM);
   const auto size_y = (PIXELS_PER_TILE / PPM);

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0, 0);
   vertices[1] = b2Vec2(0, door_height_tl * size_y);
   vertices[2] = b2Vec2(0 + size_x, door_height_tl * size_y);
   vertices[3] = b2Vec2(0 + size_x, 0);
   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   updateTransform();

   auto* fixture = _body->CreateFixture(&polygon_shape, 0);
   auto* object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeDoor);
   fixture->SetUserData(static_cast<void*>(object_data));
}

bool Door::checkPlayerAtDoor() const
{
   const auto player_pos = Player::getCurrent()->getPixelPositionFloat();
   const auto at_door = _player_at_door_rect.contains(player_pos);
   return at_door;
}

const sf::FloatRect& Door::getPixelRect() const
{
   return _pixel_rect;
}

void Door::open()
{
   if (_state == State::Opening || _state == State::Open)
   {
      return;
   }

   _state = State::Opening;

   if (_observed)
   {
      GameMechanismObserver::onEvent(getObjectId(), "doors", "state", "opening");
   }

   // play open sample
   if (_player_at_door && _sample_open)
   {
      Audio::getInstance().playSample(_sample_open.value());
   }

   // play open animation
   if (_animation_open)
   {
      _animation_open->seekToStart();
      _animation_open->play();
   }

   if (_automatic_close)
   {
      Timer::add(std::chrono::milliseconds(10000), [this]() { close(); }, Timer::Type::Singleshot, Timer::Scope::UpdateIngame);
   }
}

void Door::close()
{
   if (!_can_be_closed)
   {
      return;
   }

   if (_state == State::Closing || _state == State::Closed)
   {
      return;
   }

   _state = State::Closing;

   if (_observed)
   {
      GameMechanismObserver::onEvent(getObjectId(), "doors", "state", "closing");
   }

   // play close sample
   if (_player_at_door && _sample_close)
   {
      Audio::getInstance().playSample(_sample_close.value());
   }

   // play close animation
   if (_animation_close)
   {
      _animation_close->seekToStart();
      _animation_close->play();
   }
}

void Door::toggle()
{
   switch (_state)
   {
      case State::Open:
      {
         close();
         break;
      }
      case State::Closed:
      {
         open();
         break;
      }
      case State::Opening:
      case State::Closing:
      {
         break;
      }
   }
}

const sf::Vector2i& Door::getTilePosition() const
{
   return _tile_position_tl;
}

bool Door::isPlayerAtDoor() const
{
   return _player_at_door;
}

void Door::setPlayerAtDoor(bool player_at_door)
{
   _player_at_door = player_at_door;
}

void Door::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;
   _tile_position_tl.x = static_cast<int32_t>(x_px / PIXELS_PER_TILE);
   _tile_position_tl.y = static_cast<int32_t>(y_px / PIXELS_PER_TILE);

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;

      // read door version
      const auto type_it = map.find("version");
      if (type_it != map.end())
      {
         const auto type_identifier = type_it->second->_value_string.value();
         if (type_identifier == "version")
         {
            // this is the 1st version that has ever been implemented
            _version = Version::Version1;
         }
      }

      _observed = ValueReader::readValue<bool>("observed", map).value_or(false);

      // read z index
      const auto z_it = map.find("z");
      if (z_it != map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto texture_it = map.find("texture");
      if (texture_it != map.end())
      {
         const auto texture_path = texture_it->second->_value_string.value();
         _texture = TexturePool::getInstance().get(texture_path);
         _sprite = std::make_unique<sf::Sprite>(_texture);
         _sprite->setPosition({x_px, y_px});
      }

      const auto sample_open_it = map.find("sample_open");
      if (sample_open_it != map.end())
      {
         _sample_open = sample_open_it->second->_value_string.value();
         Audio::getInstance().addSample(_sample_open.value());
      }

      const auto sample_close_it = map.find("sample_close");
      if (sample_close_it != map.end())
      {
         _sample_close = sample_close_it->second->_value_string.value();
         Audio::getInstance().addSample(_sample_close.value());
      }

      const auto open_it = map.find("open");
      if (open_it != map.end())
      {
         const auto open = open_it->second->_value_bool.value();
         setEnabled(open);
      }
      else
      {
         setEnabled(false);
      }

      // read required key to open door
      const auto key_it = map.find("key");
      if (key_it != map.end())
      {
         const auto key = key_it->second->_value_string.value();
         _required_item = key;
      }

      // read key animation if present
      const auto offset_x = width_px * 0.5f;
      const auto offset_y = height_px * 0.5f;
      AnimationPool animation_pool{"data/sprites/door_animations.json"};
      const auto key_animation = map.find("key_animation");
      if (key_animation != map.end())
      {
         const auto key = key_animation->second->_value_string.value();
         _animation_key = animation_pool.create(key, x_px + offset_x, y_px + offset_y, false, false);
      }

      // read open animation
      const auto animation_open = map.find("animation_open");
      if (animation_open != map.end())
      {
         const auto key = animation_open->second->_value_string.value();
         _animation_open = animation_pool.create(key, x_px + offset_x, y_px + offset_y, false, false);
      }

      // read close animation
      const auto animation_close = map.find("animation_close");
      if (animation_close != map.end())
      {
         const auto key = animation_close->second->_value_string.value();
         _animation_close = animation_pool.create(key, x_px + offset_x, y_px + offset_y, false, false);

         // a property can be added if needed
         _can_be_closed = true;
      }

      _can_be_closed = ValueReader::readValue<bool>("can_be_closed", map).value_or(_can_be_closed);
   }

   // set up the old version
   if (_version == Version::Version1)
   {
      _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "doors.png");

      _door_quad[0].position.x = x_px - PIXELS_PER_TILE;
      _door_quad[0].position.y = y_px;
      _door_quad[1].position.x = x_px - PIXELS_PER_TILE;
      _door_quad[1].position.y = y_px + 3 * PIXELS_PER_TILE;
      _door_quad[2].position.x = x_px + 3 * PIXELS_PER_TILE - PIXELS_PER_TILE;
      _door_quad[2].position.y = y_px + 3 * PIXELS_PER_TILE;
      _door_quad[3].position.x = x_px + 3 * PIXELS_PER_TILE - PIXELS_PER_TILE;
      _door_quad[3].position.y = y_px;

      _pixel_rect = sf::FloatRect({x_px, y_px}, {PIXELS_PER_TILE, PIXELS_PER_TILE * 3});
   }
   else
   {
      // the first frame of the open animation should be the texture rect used for drawing
      if (_animation_open)
      {
         _sprite->setTextureRect(_animation_open->_frames.at(0));
      }
   }

   //
   // for colliding rect used to check whether the player is close to the door, extend
   // the original dimensions a little
   //
   // +///+///+///+///+
   // +---+---+---+---+
   // |///|       |///|
   // +---+       +---+
   // |///|       |///|
   // +---+       +---+
   // |///|       |///|
   // +---+---+---+---+
   // +///+///+///+///+
   //

   _player_at_door_rect.position.x = x_px - PIXELS_PER_TILE;
   _player_at_door_rect.position.y = y_px - 0.5f * PIXELS_PER_TILE;
   _player_at_door_rect.size.x = width_px + 2 * PIXELS_PER_TILE;
   _player_at_door_rect.size.y = height_px + PIXELS_PER_TILE;

   setupBody(data._world);
}
