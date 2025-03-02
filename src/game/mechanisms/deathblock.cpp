#include "game/mechanisms/deathblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/level/fixturenode.h"
#include "game/player/player.h"

// #define DEBUG_DRAW 1
#ifdef DEBUG_DRAW
#include "game/debug/debugdraw.h"
#endif

// state         frames
// spikes-open : 0,1,2,3,4,5,6,7,8,9,10,11 (speed 50)
// idle        : 12 (no speed)
// close       : 13,14,15,16,17,18,19 (speed 50)

namespace
{
//      0: retracted (spikes in)
//  1..12: extract..fully extracted (spikes out)
// 13..19: retract..almost retracted (spikes in), requires change to 0 afterwards

constexpr auto sprite_extracted = 12;
constexpr auto sprite_retracted = 19;
constexpr auto center_sprite_animation_speed = 10.0f;

constexpr auto tolerance_px = 4;
constexpr auto tolerance_px_2 = tolerance_px * 2;
}  // namespace

DeathBlock::DeathBlock(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(DeathBlock).name());
}

void DeathBlock::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& spike : _spikes)
   {
      color.draw(*spike._sprite);

#ifdef DEBUG_DRAW
      const auto& player_rect = Player::getCurrent()->getPixelRectInt();
      const auto fill_color = player_rect.intersects(spike._collision_rect_absolute) ? sf::Color::Red : sf::Color::Green;
      DebugDraw::drawRect(color, spike._collision_rect_absolute, fill_color);
#endif
   }

   color.draw(*_center_sprite);
}

void DeathBlock::setupTransform()
{
   auto x = _pixel_positions.x / PPM - (PIXELS_PER_TILE / (2 * PPM));
   auto y = _pixel_positions.y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

void DeathBlock::setupBody(const std::shared_ptr<b2World>& world)
{
   const auto size_x_m = PIXELS_PER_TILE / PPM;
   const auto size_y_m = PIXELS_PER_TILE / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0, 0);
   vertices[1] = b2Vec2(0, size_y_m);
   vertices[2] = b2Vec2(size_x_m, size_y_m);
   vertices[3] = b2Vec2(size_x_m, 0);

   // constexpr auto bevel_px = 8;
   // constexpr auto bevel_m = bevel_px * MPP;
   //
   // std::array<b2Vec2, 8> vertices{
   //    b2Vec2{bevel_m, 0.0f},
   //    b2Vec2{0.0f, bevel_m},
   //    b2Vec2{0.0f, size_y_m - bevel_m},
   //    b2Vec2{bevel_m, size_y_m},
   //    b2Vec2{size_x_m - bevel_m, size_y_m},
   //    b2Vec2{size_x_m, size_y_m - bevel_m},
   //    b2Vec2{size_x_m, bevel_m},
   //    b2Vec2{size_x_m - bevel_m, 0.0f},
   // };

   b2PolygonShape polygon_shape;
   polygon_shape.Set(vertices, 4);

   // polygon_shape.Set(vertices.data(), static_cast<int32_t>(vertices.size()));

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   setupTransform();

   auto* fixture = _body->CreateFixture(&polygon_shape, 0);
   auto* object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeDeathBlock);
   fixture->SetUserData(static_cast<void*>(object_data));
}

void DeathBlock::updateLeverLag(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (_lever_lag <= 0.0f)
      {
         _lever_lag = 0.0f;
      }
      else
      {
         _lever_lag -= dt.asSeconds();
      }
   }
   else
   {
      if (_lever_lag < 1.0f)
      {
         _lever_lag += dt.asSeconds();
      }
      else
      {
         _lever_lag = 1.0f;
      }
   }
}

void DeathBlock::updateCollision()
{
   // check for intersection with player
   const auto& player_rect = Player::getCurrent()->getPixelRectInt();
   const auto x_px = static_cast<int32_t>(_body->GetPosition().x * PPM - PIXELS_PER_TILE);
   const auto y_px = static_cast<int32_t>(_body->GetPosition().y * PPM - PIXELS_PER_TILE);

   // want a copy of the original rect
   int32_t index = 0;
   for (auto& spike : _spikes)
   {
      spike._collision_rect_absolute.position.x = spike._collision_rect_relative.position.x + x_px;
      spike._collision_rect_absolute.position.y = spike._collision_rect_relative.position.y + y_px;

      const auto deadly = (spike._state == Spike::State::Extracted);

      if (player_rect.findIntersection(spike._collision_rect_absolute).has_value() && deadly)
      {
         Player::getCurrent()->damage(_damage);
      }

      ++index;
   }
}

DeathBlock::Spike::Spike()
{
   _collision_rect_absolute.size.x = PIXELS_PER_TILE - tolerance_px_2;
   _collision_rect_absolute.size.y = PIXELS_PER_TILE - tolerance_px_2;
}

bool DeathBlock::Spike::hasChanged() const
{
   return _sprite_index != _sprite_index_prev;
}

void DeathBlock::Spike::updateIndex()
{
   _sprite_index_prev = _sprite_index;
   _sprite_index = static_cast<int32_t>(_state_time_s);
}

void DeathBlock::Spike::extract(const sf::Time& dt)
{
   constexpr auto animation_speed_factor = 40.0f;

   _state_time_s += animation_speed_factor * dt.asSeconds();

   if (_state_time_s >= sprite_extracted)
   {
      _wait_time = sf::seconds(0);
      _state = Spike::State::Extracted;
   }
}

void DeathBlock::Spike::extracted(const sf::Time& dt, const sf::Time& time_on)
{
   _wait_time += dt;
   if (_wait_time > time_on)
   {
      _state = Spike::State::Retracting;
   };
}

void DeathBlock::Spike::retract(const sf::Time& dt)
{
   constexpr auto animation_speed_factor = 40.0f;

   _state_time_s += animation_speed_factor * dt.asSeconds();

   if (_state_time_s >= sprite_retracted)
   {
      _state_time_s = 0;  // reset after one cycle
      _wait_time = sf::seconds(0);
      _state = Spike::State::Retracted;
   }
}

void DeathBlock::Spike::retracted(const sf::Time& dt, const sf::Time& time_off)
{
   _wait_time += dt;
   if (_wait_time > time_off)
   {
      _state = Spike::State::Extracting;
   };
}

void DeathBlock::updateStatesInterval(const sf::Time& dt)
{
   for (auto& spike : _spikes)
   {
      auto& time = spike._wait_offset;
      if (time.asSeconds() > 0.0f)
      {
         time -= dt;
         continue;
      }

      switch (spike._state)
      {
         case Spike::State::Extracting:
         {
            spike.extract(dt);
            break;
         }
         case Spike::State::Retracting:
         {
            spike.retract(dt);
            break;
         }
         case Spike::State::Extracted:
         {
            spike.extracted(dt, _time_on);
            break;
         }
         case Spike::State::Retracted:
         {
            spike.retracted(dt, _time_off);
            break;
         }
      }
   }
}

void DeathBlock::updateStatesRotate(const sf::Time& dt)
{
   const auto spike_index = _rotation[_spike_rotation_counter % 4];
   auto& spike = _spikes[spike_index];

   switch (spike._state)
   {
      case Spike::State::Extracting:
      {
         spike.extract(dt);
         break;
      }
      case Spike::State::Retracting:
      {
         spike.retract(dt);
         break;
      }
      case Spike::State::Extracted:
      {
         spike.extracted(dt, _time_on);
         break;
      }
      case Spike::State::Retracted:
      {
         spike.retracted(dt, _time_off);

         if (spike._state == Spike::State::Extracting)
         {
            _spike_rotation_counter++;
         }

         break;
      }
   }
}

void DeathBlock::updateStates(const sf::Time& dt)
{
   if (_time_offset.asSeconds() > 0.0f)
   {
      _time_offset -= dt;
      return;
   }

   switch (_mode)
   {
      case Mode::AlwaysOn:
      {
         for (auto& spike : _spikes)
         {
            // spike._sprite_index = sprite_extracted;
            spike._state_time_s = sprite_extracted;
            spike._state = Spike::State::Extracted;
         }
         break;
      }
      case Mode::Interval:
      {
         updateStatesInterval(dt);
         break;
      }
      case Mode::Rotate:
      {
         updateStatesRotate(dt);
         break;
      }
      case Mode::Invalid:
      case Mode::OnContact:
      {
         // not implemented
         break;
      }
   }

   // update center
   _center_sprite_time_s += dt.asSeconds() * center_sprite_animation_speed;
   _center_sprite_index = static_cast<int32_t>(_center_sprite_time_s) % sprite_retracted;

   // update resulting spriteset offsets
   for (auto& spike : _spikes)
   {
      spike.updateIndex();
   }
}

void DeathBlock::updateBoundingBox()
{
   _rect.position.x = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   _rect.position.y = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   _rect.size.x = 3 * PIXELS_PER_TILE;
   _rect.size.y = 3 * PIXELS_PER_TILE;
}

void DeathBlock::updateSprites()
{
   const auto x = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   const auto y = _body->GetPosition().y * PPM - PIXELS_PER_TILE;
   const auto tl_px = PIXELS_PER_TILE * 3;

   int32_t row = 1;  // first row is reserved for center
   for (auto& spike : _spikes)
   {
      if (spike.hasChanged())
      {
         spike._sprite->setTextureRect(sf::IntRect({spike._sprite_index * tl_px, tl_px * row}, {tl_px, tl_px}));
      }

      spike._sprite->setPosition({x, y});
      row++;
   }

   _center_sprite->setTextureRect(sf::IntRect({_center_sprite_index * tl_px, 0}, {tl_px, tl_px}));
   _center_sprite->setPosition({x, y});
}

void DeathBlock::updatePosition(const sf::Time& dt)
{
   const auto _movement_speed = 1.0f;
   const auto movement_delta = dt.asSeconds() * _velocity * _movement_speed;
   _interpolation.updateTime(movement_delta);
   const auto current_position = _body->GetPosition();
   const auto target_position = _interpolation.computePosition(_interpolation.getTime());
   const auto direction = target_position - current_position;
   constexpr auto timestep = TIMESTEP_ERROR * (PPM / 60.0f);
   _body->SetTransform(target_position, 0.0f);
   _body->SetLinearVelocity(timestep * direction);

   const auto dx = target_position - current_position;
   auto& platform = Player::getCurrent()->getPlatform();
   if (platform.getPlatformBody() == _body)
   {
      platform.setPlatformDx(dx.x);
      platform.setGravityScale(1.0f);
   }
}

void DeathBlock::update(const sf::Time& dt)
{
   updateLeverLag(dt);
   updateStates(dt);
   updatePosition(dt);
   updateSprites();
   updateBoundingBox();
   updateCollision();
}

std::optional<sf::FloatRect> DeathBlock::getBoundingBoxPx()
{
   return _rect;
}

void DeathBlock::setup(const GameDeserializeData& data)
{
   std::map<std::string, std::shared_ptr<TmxProperty>> default_property_map;
   const auto& map = data._tmx_object->_properties ? data._tmx_object->_properties->_map : default_property_map;

   _texture = TexturePool::getInstance().get("data/sprites/enemy_deathblock.png");

   for (auto& spike : _spikes)
   {
      spike._sprite = std::make_unique<sf::Sprite>(*_texture);
   }

   _spikes[Spike::Orientation::Up]._collision_rect_relative = sf::IntRect{
      {1 * PIXELS_PER_TILE + tolerance_px, 0 * PIXELS_PER_TILE + tolerance_px},
      {PIXELS_PER_TILE - tolerance_px_2, PIXELS_PER_TILE - tolerance_px_2}
   };

   _spikes[Spike::Orientation::Right]._collision_rect_relative = sf::IntRect{
      {2 * PIXELS_PER_TILE + tolerance_px, 1 * PIXELS_PER_TILE + tolerance_px},
      {PIXELS_PER_TILE - tolerance_px_2, PIXELS_PER_TILE - tolerance_px_2}
   };

   _spikes[Spike::Orientation::Down]._collision_rect_relative = sf::IntRect{
      {1 * PIXELS_PER_TILE + tolerance_px, 2 * PIXELS_PER_TILE + tolerance_px},
      {PIXELS_PER_TILE - tolerance_px_2, PIXELS_PER_TILE - tolerance_px_2}
   };

   _spikes[Spike::Orientation::Left]._collision_rect_relative = sf::IntRect{
      {0 * PIXELS_PER_TILE + tolerance_px, 1 * PIXELS_PER_TILE + tolerance_px},
      {PIXELS_PER_TILE - tolerance_px_2, PIXELS_PER_TILE - tolerance_px_2}
   };

   _center_sprite = std::make_unique<sf::Sprite>(*_texture);

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   _pixel_positions.x = data._tmx_object->_x_px;
   _pixel_positions.y = data._tmx_object->_y_px;

   _time_off = sf::seconds(ValueReader::readValue<float>("time_off", map).value_or(2.0f));
   _time_on = sf::seconds(ValueReader::readValue<float>("time_on", map).value_or(0.2f));
   _time_offset = sf::seconds(ValueReader::readValue<float>("time_offset", map).value_or(0.0f));
   _damage = ValueReader::readValue<int32_t>("damage", map).value_or(100);
   const auto mode = ValueReader::readValue<std::string>("mode", map).value_or("always_on");

   if (mode == "always_on")
   {
      _mode = Mode::AlwaysOn;
   }
   else if (mode == "interval")
   {
      _mode = Mode::Interval;
   }
   else if (mode == "rotate")
   {
      _mode = Mode::Rotate;
      _spikes[0]._active = true;
      _spikes[1]._active = false;
      _spikes[2]._active = false;
      _spikes[3]._active = false;
   }

   setupBody(data._world);

   // setup velocity
   const auto velocity = ValueReader::readValue<float>("velocity", map).value_or(50.0f);
   auto pixel_path = data._tmx_object->_polyline ? data._tmx_object->_polyline->_polyline : data._tmx_object->_polygon->_polyline;
   const auto start_pos = pixel_path.at(0);
   pixel_path.push_back(start_pos);
   _velocity = 50.0f / SfmlMath::length(pixel_path);

   // setup path
   auto pos_index = 0;
   for (const auto& poly_pos : pixel_path)
   {
      b2Vec2 world_pos;
      const auto time = pos_index / static_cast<float>(pixel_path.size() - 1);

      const auto x = (data._tmx_object->_x_px + poly_pos.x - (PIXELS_PER_TILE) / 2.0f) * MPP;
      const auto y = (data._tmx_object->_y_px + poly_pos.y - (PIXELS_PER_TILE) / 2.0f) * MPP;

      world_pos.x = x;
      world_pos.y = y;

      _interpolation.addKey(world_pos, time);

      pos_index++;
   }

   updateSprites();
}
