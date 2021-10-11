#include "moveablebox.h"

#include "constants.h"
#include "fixturenode.h"
#include "framework/tmxparser/tmxobject.h"
#include "texturepool.h"

#include <iostream>


//--------------------------------------------------------------------------------------------------
MoveableBox::MoveableBox(GameNode* node)
 : GameNode(node)
{
   setName("MoveableBox");

   _texture = TexturePool::getInstance().get("data/level-malte/tilesets/crypts.png");
   _sprite.setTexture(*_texture.get());
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{
   const auto x = _body->GetPosition().x * PPM;
   const auto y = _body->GetPosition().y * PPM;
   _sprite.setPosition(x, y - 24);
}


// box: pos: 5160 x 1056 size: 48 x 48
// box: pos: 5376 x 1080 size: 24 x 24



/*

   +----+
   | __ |
   |//\\|
   +----+
   |####|
   |####|
   +----+

   +----+----+
   | ___|___ |
   |////|\\\\|
   +----+----+
   |####|####|
   |####|####|
   +----+----+
   |####|####|
   |####|####|
   +----+----+


*/


//--------------------------------------------------------------------------------------------------
void MoveableBox::setup(TmxObject* tmx_object, const std::shared_ptr<b2World>& world)
{
   _size.x = tmx_object->_width_px;
   _size.y = tmx_object->_height_px;

   _sprite.setPosition(tmx_object->_x_px, tmx_object->_y_px - 24);

   switch (static_cast<int32_t>(_size.x))
   {
      case 24:
      {
         _sprite.setTextureRect(sf::IntRect(1392, 0, 24, 2 * 24));
         break;
      }

      case 48:
      {
         _sprite.setTextureRect(sf::IntRect(1296, 24, 2 * 24, 3 * 24));
         break;
      }

      default:
      {
         break;
      }
   }

   setupBody(world);
   setupTransform();
}


//-----------------------------------------------------------------------------
void MoveableBox::setupTransform()
{
   auto x = _sprite.getPosition().x / PPM;
   auto y = _sprite.getPosition().y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;
   auto size_x = _size.x / PPM;
   auto size_y = _size.y / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,     0);
   vertices[1] = b2Vec2(0,     size_y);
   vertices[2] = b2Vec2(size_x, size_y);
   vertices[3] = b2Vec2(size_x, 0);

   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_dynamicBody;
   _body = world->CreateBody(&body_def);
   // mBody->SetGravityScale(0.0f);

   setupTransform();

   auto fixture = _body->CreateFixture(&polygon_shape, 0);
   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMoveableBox);
   fixture->SetUserData(static_cast<void*>(object_data));
}

