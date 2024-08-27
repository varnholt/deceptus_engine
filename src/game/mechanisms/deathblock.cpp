#include "game/mechanisms/deathblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/level/fixturenode.h"
#include "game/player/player.h"

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
}  // namespace

// #define DEBUG_DRAW 1

DeathBlock::DeathBlock(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(DeathBlock).name());
}

void DeathBlock::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& spike : _spikes)
   {
      color.draw(spike._sprite);

#ifdef DEBUG_DRAW
      DebugDraw::drawRect(color, spike._collision_rect_absolute);
#endif
   }

   color.draw(_center_sprite);
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

   b2PolygonShape polygon_shape;
   polygon_shape.Set(vertices, 4);

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
      spike._collision_rect_absolute.left = spike._collision_rect_relative.left + x_px;
      spike._collision_rect_absolute.top = spike._collision_rect_relative.top + y_px;

      const auto deadly = (spike._state == Spike::State::Extracted);

      if (player_rect.intersects(spike._collision_rect_absolute) && deadly)
      {
         Player::getCurrent()->damage(1);
      }

      ++index;
   }
}

void DeathBlock::updateStatesInterval(const sf::Time& dt)
{
   constexpr auto animation_speed_factor = 40.0f;

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
            spike._state_time_s += animation_speed_factor * dt.asSeconds();

            if (spike._state_time_s >= sprite_extracted)
            {
               spike._wait_time = sf::seconds(0);
               spike._state = Spike::State::Extracted;
            }

            break;
         }
         case Spike::State::Retracting:
         {
            spike._state_time_s += animation_speed_factor * dt.asSeconds();

            if (spike._state_time_s >= sprite_retracted)
            {
               spike._state_time_s = 0;  // reset after one cycle
               spike._wait_time = sf::seconds(0);
               spike._state = Spike::State::Retracted;
            }

            break;
         }
         case Spike::State::Extracted:
         {
            spike._wait_time += dt;
            if (spike._wait_time > _time_on)
            {
               spike._state = Spike::State::Retracting;
            };

            break;
         }
         case Spike::State::Retracted:
         {
            spike._wait_time += dt;
            if (spike._wait_time > _time_off)
            {
               spike._state = Spike::State::Extracting;
            };

            break;
         }
      }
   }
}

void DeathBlock::updateStates(const sf::Time& dt)
{
   switch (_mode)
   {
      case Mode::AlwaysOn:
      {
         for (auto& spike : _spikes)
         {
            spike._sprite_index = sprite_extracted;
         }
         break;
      }
      case Mode::Interval:
      {
         updateStatesInterval(dt);
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
   _center_sprite_time_s += dt.asSeconds();
   _center_sprite_index = static_cast<int32_t>(_center_sprite_time_s) % sprite_retracted;

   // update resulting spriteset offsets
   for (auto& spike : _spikes)
   {
      spike.updateIndex();
   }
}

void DeathBlock::updateBoundingBox()
{
   _rect.left = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   _rect.top = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   _rect.width = 3 * PIXELS_PER_TILE;
   _rect.height = 3 * PIXELS_PER_TILE;
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
         spike._sprite.setTextureRect(sf::IntRect(spike._sprite_index * tl_px, tl_px * row, tl_px, tl_px));
      }

      spike._sprite.setPosition(x, y);
      row++;
   }

   _center_sprite.setTextureRect(sf::IntRect(_center_sprite_index * tl_px, 0, tl_px, tl_px));
   _center_sprite.setPosition(x, y);
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
   _texture = TexturePool::getInstance().get("data/sprites/enemy_deathblock.png");

   for (auto& spike : _spikes)
   {
      spike._sprite.setTexture(*_texture);
   }

   // offsets:
   //
   //    up: 1, 0
   //    right: 2, 1
   //    down: 1, 2
   //    left: 0, 1

   _spikes[Spike::Orientation::Up]._collision_rect_relative =
      sf::IntRect{1 * PIXELS_PER_TILE, 0 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};
   _spikes[Spike::Orientation::Right]._collision_rect_relative =
      sf::IntRect{2 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};
   _spikes[Spike::Orientation::Down]._collision_rect_relative =
      sf::IntRect{1 * PIXELS_PER_TILE, 2 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};
   _spikes[Spike::Orientation::Left]._collision_rect_relative =
      sf::IntRect{0 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE};

   _center_sprite.setTexture(*_texture);

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   _pixel_positions.x = data._tmx_object->_x_px;
   _pixel_positions.y = data._tmx_object->_y_px;

   _time_off = sf::seconds(2.0f);
   _time_on = sf::seconds(0.2f);

   // constexpr auto delta_time = 2.0f / 3.0f;
   // _wait_offsets[0] = sf::seconds(delta_time * 0);
   // _wait_offsets[1] = sf::seconds(delta_time * 1);
   // _wait_offsets[2] = sf::seconds(delta_time * 2);
   // _wait_offsets[3] = sf::seconds(delta_time * 3);

   setupBody(data._world);

   auto pixel_path = data._tmx_object->_polyline ? data._tmx_object->_polyline->_polyline : data._tmx_object->_polygon->_polyline;
   const auto start_pos = pixel_path.at(0);
   pixel_path.push_back(start_pos);
   _velocity = 50.0f / SfmlMath::length(pixel_path);

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
      _pixel_paths.emplace_back((start_pos.x + data._tmx_object->_x_px), (start_pos.y + data._tmx_object->_y_px));

      // Log::Info() << "world: " << x << ", " << y << " pixel: " << tmxObject->mX << ", " << tmxObject->mY;

      pos_index++;
   }

   updateSprites();
}
