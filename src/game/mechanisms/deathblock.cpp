#include "deathblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"

#include "constants.h"
#include "fixturenode.h"
#include "player/player.h"
#include "texturepool.h"

#include <iostream>


DeathBlock::DeathBlock(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(DeathBlock).name());
}


void DeathBlock::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& sprite : _sprites)
   {
      color.draw(sprite);
   }
}


// enemy_deathblock
// 14 animation cycles
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


//-----------------------------------------------------------------------------
void DeathBlock::setupTransform()
{
   auto x = _pixel_positions.x / PPM - (PIXELS_PER_TILE / (2 * PPM));
   auto y = _pixel_positions.y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void DeathBlock::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;

   auto size_x = PIXELS_PER_TILE / PPM;
   auto size_y = PIXELS_PER_TILE / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,      0);
   vertices[1] = b2Vec2(0,      size_y);
   vertices[2] = b2Vec2(size_x, size_y);
   vertices[3] = b2Vec2(size_x, 0);

   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   setupTransform();

   auto fixture = _body->CreateFixture(&polygon_shape, 0);
   auto object_data = new FixtureNode(this);
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
    auto player_rect = Player::getCurrent()->getPixelRectInt();

    auto x = static_cast<int32_t>(_body->GetPosition().x * PPM - PIXELS_PER_TILE);
    auto y = static_cast<int32_t>(_body->GetPosition().y * PPM - PIXELS_PER_TILE);

    // want a copy of the original rect
    for (auto rect : _collision_rects)
    {
        rect.left += x;
        rect.top += y;

        if (player_rect.intersects(rect))
        {
           Player::getCurrent()->damage(100);
        }
    }
}


void DeathBlock::update(const sf::Time& dt)
{
   updateLeverLag(dt);

   _interpolation.update(_body->GetPosition());
   {
      _body->SetLinearVelocity(_lever_lag * TIMESTEP_ERROR * (PPM / 60.0f) * _interpolation.getVelocity());
   }

   for (auto i = 0u; i < _sprites.size(); i++)
   {
      _sprites[i].setTextureRect(
         sf::IntRect(
            _offsets[i].x * PIXELS_PER_TILE + _states[i] * PIXELS_PER_TILE,
            _offsets[i].y * PIXELS_PER_TILE + _states[i] * PIXELS_PER_TILE,
            PIXELS_PER_TILE,
            PIXELS_PER_TILE
         )
      );

      // need to move by one tile because the center is not 0, 0 but -24, -24
      auto x = _body->GetPosition().x * PPM + _offsets[i].x * PIXELS_PER_TILE - PIXELS_PER_TILE;
      auto y = _body->GetPosition().y * PPM + _offsets[i].y * PIXELS_PER_TILE - PIXELS_PER_TILE;

      _sprites[i].setPosition(x, y);
   }

   updateCollision();
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

   std::vector<sf::Vector2f> pixel_path = data._tmx_object->_polyline->_polyline;
   auto pos = pixel_path.at(0);

   auto i = 0;
   for (const auto& poly_pos : pixel_path)
   {
      b2Vec2 world_pos;
      auto time = i / static_cast<float>(pixel_path.size() - 1);

      auto x = (data._tmx_object->_x_px + poly_pos.x - (PIXELS_PER_TILE) / 2.0f) * MPP;
      auto y = (data._tmx_object->_y_px + poly_pos.y - (PIXELS_PER_TILE) / 2.0f) * MPP;

      world_pos.x = x;
      world_pos.y = y;

      _interpolation.addKey(world_pos, time);
      _pixel_paths.push_back({(pos.x + data._tmx_object->_x_px), (pos.y + data._tmx_object->_y_px)});

      // Log::Info() << "world: " << x << ", " << y << " pixel: " << tmxObject->mX << ", " << tmxObject->mY;

      i++;
   }
}
