#include "conveyorbelt.h"
#include "player/player.h"
#include "texturepool.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"

#include <iostream>

namespace
{
static const auto Y_OFFSET = -10;
static const auto BELT_TILE_COUNT = 8;
static const auto ARROW_INDEX_X = 11;
static const auto ARROW_INDEX_LEFT_Y = 0;
static const auto ARROW_INDEX_RIGHT_Y = 1;
}


std::vector<b2Body*> ConveyorBelt::__bodies_on_belt;


void ConveyorBelt::setVelocity(float velocity)
{
   _velocity = velocity;
   _points_right = (_velocity > 0.0f);
}


void ConveyorBelt::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& sprite : _belt_sprites)
   {
      color.draw(sprite);
   }

// disable for now
//    for (auto& sprite : mArrowSprites)
//    {
//       target.draw(sprite);
//    }
}


void ConveyorBelt::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (_lever_lag <= 0.0f)
      {
         return;
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

   _elapsed += _lever_lag * dt.asSeconds();
   updateSprite();
}


void ConveyorBelt::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
   _lever_lag = enabled ? 0.0f : 1.0f;
}


void ConveyorBelt::updateSprite()
{
   const auto val = static_cast<int32_t>(_elapsed * 40.0f * fabs(_velocity)) % BELT_TILE_COUNT;
   const auto xOffset = _points_right ? 7 - val :  val;
   auto yOffset = 0u;

   for (auto i = 0u; i < _belt_sprites.size(); i++)
   {
       if (i == 0u)
       {
           // left tile (row 0)
           yOffset = 0;
       }
       else if (i == _belt_sprites.size() - 1)
       {
           // right tile (row 2)
           yOffset = PIXELS_PER_TILE * 2;
       }
       else
       {
           // middle tile (row 1)
           yOffset = PIXELS_PER_TILE;
       }

       _belt_sprites[i].setTextureRect({
            xOffset * PIXELS_PER_TILE,
            static_cast<int32_t>(yOffset),
            PIXELS_PER_TILE,
            PIXELS_PER_TILE
         }
      );
   }
}


ConveyorBelt::ConveyorBelt(
   GameNode* parent,
   const std::shared_ptr<b2World>& world,
   TmxObject* tmx_object,
   const std::filesystem::path& base_path
)
 : FixtureNode(parent)
{
   setName(typeid(ConveyorBelt).name());
   setType(ObjectTypeConveyorBelt);

   _texture = TexturePool::getInstance().get(base_path / "tilesets" / "cbelt.png");

   const auto x         = tmx_object->_x_px;
   const auto y         = tmx_object->_y_px;
   const auto width_px  = tmx_object->_width_px;
   const auto height_px = tmx_object->_height_px;

   auto velocity = _velocity;

   if (tmx_object->_properties)
   {
      auto velocity_it = tmx_object->_properties->_map.find("velocity");
      if (velocity_it != tmx_object->_properties->_map.end())
      {
         velocity = velocity_it->second->_value_float.value();
      }
   }

   setVelocity(velocity);

   _position_b2d = b2Vec2(x * MPP, y * MPP);
   _position_sfml.x = x;
   _position_sfml.y = y;

   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = _position_b2d;
   _body = world->CreateBody(&body_def);

   // auto halfPhysicsWidth = width * MPP * 0.5f;
   // auto halfPhysicsHeight = height * MPP * 0.5f;
   //
   // create fixture for physical boundaries of the belt object
   // mShapeBounds.SetAsBox(
   //    halfPhysicsWidth, halfPhysicsHeight,
   //    b2Vec2(halfPhysicsWidth, halfPhysicsHeight),
   //    0.0f
   // );

   const auto width_m  = width_px * MPP;
   const auto height_m = height_px * MPP;

   constexpr auto dx = 0.002f;
   constexpr auto dy = 0.001f;
   std::array<b2Vec2, 6> vertices {
      b2Vec2{dx,                0.0},
      b2Vec2{0.0,               height_m - dy},
      b2Vec2{0.0,               height_m},
      b2Vec2{width_m,       height_m},
      b2Vec2{width_m,       height_m - dy},
      b2Vec2{width_m - dx,  0.0}
   };

   _shape.Set(vertices.data(), static_cast<int32_t>(vertices.size()));

   b2FixtureDef boundary_fixture_def;
   boundary_fixture_def.shape = &_shape;
   boundary_fixture_def.density = 1.0f;
   boundary_fixture_def.isSensor = false;
   auto boundary_fixture = _body->CreateFixture(&boundary_fixture_def);
   boundary_fixture->SetUserData(static_cast<void*>(this));

   _belt_pixel_rect.left   = static_cast<int32_t>(x);
   _belt_pixel_rect.top    = static_cast<int32_t>(y);
   _belt_pixel_rect.height = static_cast<int32_t>(height_px);
   _belt_pixel_rect.width  = static_cast<int32_t>(width_px);

   static auto ROUND_EPSILON = 0.5f;
   auto tile_count = static_cast<uint32_t>( (width_px / PIXELS_PER_TILE) + ROUND_EPSILON);
   // Log::Info() << "estimating " << tileCount << " tiles per belt" << " at " << x << ", " << y;

   for (auto i = 0u; i < tile_count; i++)
   {
      sf::Sprite belt_sprite;
      belt_sprite.setTexture(*_texture);
      belt_sprite.setPosition(
         x + i * PIXELS_PER_TILE,
         y + Y_OFFSET
      );

      _belt_sprites.push_back(belt_sprite);
   }

   for (auto i = 0u; i < tile_count - 1; i++)
   {
      sf::Sprite arrow_sprite;
      arrow_sprite.setTexture(*_texture);
      arrow_sprite.setPosition(
         x + i * PIXELS_PER_TILE + 12,
         y - 12
      );

      arrow_sprite.setTextureRect({
           ARROW_INDEX_X * PIXELS_PER_TILE,
           (velocity < -0.0001 ? ARROW_INDEX_LEFT_Y : ARROW_INDEX_RIGHT_Y) * PIXELS_PER_TILE,
           PIXELS_PER_TILE,
           PIXELS_PER_TILE
        }
      );

      _arrow_sprites.push_back(arrow_sprite);
   }

   updateSprite();

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin));
}


b2Body *ConveyorBelt::getBody() const
{
   return _body;
}


float ConveyorBelt::getVelocity() const
{
   return _velocity;
}


void ConveyorBelt::update()
{
   __bodies_on_belt.clear();
   auto player = Player::getCurrent();
   player->setBeltVelocity(0.0f);
   player->setOnBelt(false);
}


void ConveyorBelt::processFixtureNode(
   FixtureNode* fixture_node,
   b2Body* colliding_body
)
{
   if (fixture_node->getType() == ObjectTypeConveyorBelt)
   {
      auto player_body = Player::getCurrent()->getBody();

      auto belt = dynamic_cast<ConveyorBelt*>(fixture_node);

      if (!belt->isEnabled())
      {
         return;
      }

      auto belt_velocity = belt->getVelocity();

      // only process a body once since bodies can have multiple fixtures
      if (std::find(__bodies_on_belt.begin(), __bodies_on_belt.end(), colliding_body) == __bodies_on_belt.end())
      {
         auto velocity = colliding_body->GetLinearVelocity();
         velocity.x += belt_velocity;

        if (colliding_body != player_body)
        {
           colliding_body->SetLinearVelocity(velocity);
           __bodies_on_belt.push_back(colliding_body);
        }
        else
        {
           // handle player differently because multiple linear velocities are applied to the player
           auto player = Player::getCurrent();
           player->setOnBelt(true);
           player->setBeltVelocity(belt_velocity);
        }
      }
   }
}


sf::IntRect ConveyorBelt::getPixelRect() const
{
   return _belt_pixel_rect;
}


void ConveyorBelt::processContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   if (fixtureUserDataA)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataA);
      processFixtureNode(fixtureNode, contact->GetFixtureB()->GetBody());
   }

   if (fixtureUserDataB)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);
      processFixtureNode(fixtureNode, contact->GetFixtureA()->GetBody());
   }
}


