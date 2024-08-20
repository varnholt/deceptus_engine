#include "game/mechanisms/deathblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/level/fixturenode.h"
#include "game/player/player.h"

#define DEBUG_DRAW 1

DeathBlock::DeathBlock(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(DeathBlock).name());
}

void DeathBlock::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& sprite : _sprites)
   {
      color.draw(sprite);
   }

#ifdef DEBUG_DRAW
   const auto x_px = static_cast<int32_t>(_body->GetPosition().x * PPM - PIXELS_PER_TILE);
   const auto y_px = static_cast<int32_t>(_body->GetPosition().y * PPM - PIXELS_PER_TILE);
   for (auto rect : _collision_rects)
   {
      rect.left += x_px;
      rect.top += y_px;
      DebugDraw::drawRect(color, rect);
   }
#endif
}

// enemy_deathblock
// 24 animation cycles

//      0: retracted (spikes in)
//  1..19: extract..fully extracted (spikes out)
// 20..24: retract..almost retracted (spikes in), requires change to 0 afterwards

// 0: spikes out
// 13: spikes in
//
// sprite setup:
//
//           +---+
//           | 0 |
//       +---+---+---+
//       | 1 | 2 | 3 |
//       +---+---+---+
//           | 4 |
//           +---+
//
// offsets:
//
//    0: 1, 0
//    1: 0, 1
//    2: 1, 1
//    3: 2, 1
//    4: 1, 2

void DeathBlock::setupTransform()
{
   auto x = _pixel_positions.x / PPM - (PIXELS_PER_TILE / (2 * PPM));
   auto y = _pixel_positions.y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

void DeathBlock::setupBody(const std::shared_ptr<b2World>& world)
{
   auto size_x_m = PIXELS_PER_TILE / PPM;
   auto size_y_m = PIXELS_PER_TILE / PPM;

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
   for (auto rect : _collision_rects)
   {
      rect.left += x_px;
      rect.top += y_px;

      if (player_rect.intersects(rect))
      {
         // check the 4 sub-bounding boxes here and check if the spikes are actually extraced

         Player::getCurrent()->damage(1);
      }
   }
}

void DeathBlock::updateStates(const sf::Time& dt)
{
   switch (_mode)
   {
      case Mode::AlwaysOn:
      {
         _sprite_indices[SpikeOrientation::Up] = 19;
         _sprite_indices[SpikeOrientation::Right] = 19;
         _sprite_indices[SpikeOrientation::Down] = 19;
         _sprite_indices[SpikeOrientation::Left] = 19;
         break;
      }
      case Mode::Interval:
      {
         for (int32_t index = 0; index < _states.size(); ++index)
         {
            const auto state = _states[index];
            switch (state)
            {
               case State::Extracting:
               {
                  _state_times[index] += dt.asSeconds();

                  if (_state_times[index] > 18)
                  {
                     _state_times[index] = 0;
                     _states[index] = State::Extracted;
                  }

                  break;
               }
               case State::Retracting:
               {
                  _state_times[index] += dt.asSeconds();

                  if (_state_times[index] > 25)
                  {
                     _state_times[index] = 0;
                     _states[index] = State::Retracted;
                  }
                  break;
               }
               case State::Extracted:
               {
                  auto& time = _times[index];
                  time += dt;

                  if (time > sf::seconds(1.0f))
                  {
                     _times[index] = sf::seconds(0.0f);
                     _states[index] = State::Retracting;
                  };

                  break;
               }
               case State::Retracted:
               {
                  auto& time = _times[index];
                  time += dt;

                  if (time > sf::seconds(1.0f))
                  {
                     _times[index] = sf::seconds(0.0f);
                     _states[index] = State::Extracting;
                  };

                  break;
               }
            }
         }

         break;
      }
      case Mode::Invalid:
      case Mode::OnContact:
      {
         // not implemented
         break;
      }
   }

   _state_times[SpikeOrientation::Center] += dt.asSeconds();

   _sprite_indices[SpikeOrientation::Center] = static_cast<int32_t>(_state_times[SpikeOrientation::Center]) % 6;
   _sprite_indices[SpikeOrientation::Up] = static_cast<int32_t>(_state_times[SpikeOrientation::Up]);
   _sprite_indices[SpikeOrientation::Right] = static_cast<int32_t>(_state_times[SpikeOrientation::Right]);
   _sprite_indices[SpikeOrientation::Down] = static_cast<int32_t>(_state_times[SpikeOrientation::Down]);
   _sprite_indices[SpikeOrientation::Left] = static_cast<int32_t>(_state_times[SpikeOrientation::Left]);
}

void DeathBlock::updateBoundingBox()
{
   _rect.left = _body->GetPosition().x * PPM - PIXELS_PER_TILE;  // + _offsets[1].x * PIXELS_PER_TILE;
   _rect.top = _body->GetPosition().x * PPM - PIXELS_PER_TILE;   // + _offsets[0].y * PIXELS_PER_TILE;
   _rect.width = 3 * PIXELS_PER_TILE;
   _rect.height = 3 * PIXELS_PER_TILE;
}

void DeathBlock::updateSprites()
{
   const auto x = _body->GetPosition().x * PPM - PIXELS_PER_TILE;
   const auto y = _body->GetPosition().y * PPM - PIXELS_PER_TILE;
   const auto tl_px = PIXELS_PER_TILE * 3;

   // first row is not used
   _sprites[SpikeOrientation::Up].setTextureRect(sf::IntRect(_sprite_indices[SpikeOrientation::Up] * tl_px, tl_px, tl_px, tl_px));
   _sprites[SpikeOrientation::Down].setTextureRect(sf::IntRect(_sprite_indices[SpikeOrientation::Down] * tl_px, tl_px * 2, tl_px, tl_px));
   _sprites[SpikeOrientation::Right].setTextureRect(sf::IntRect(_sprite_indices[SpikeOrientation::Right] * tl_px, tl_px * 3, tl_px, tl_px));
   _sprites[SpikeOrientation::Left].setTextureRect(sf::IntRect(_sprite_indices[SpikeOrientation::Left] * tl_px, tl_px * 4, tl_px, tl_px));
   _sprites[SpikeOrientation::Center].setTextureRect(sf::IntRect(_sprite_indices[SpikeOrientation::Center] * tl_px, tl_px * 5, tl_px, tl_px)
   );

   for (auto& sprite : _sprites)
   {
      sprite.setPosition(x, y);
   }
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

   for (auto& sprite : _sprites)
   {
      sprite.setTexture(*_texture);
   }

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   _pixel_positions.x = data._tmx_object->_x_px;
   _pixel_positions.y = data._tmx_object->_y_px;

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
}
