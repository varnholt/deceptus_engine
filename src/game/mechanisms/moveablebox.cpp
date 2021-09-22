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
void MoveableBox::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   //   std::cout
   //      << "box: pos: " << tmxObject->mX << " x " << tmxObject->mY
   //      << " size: " << tmxObject->mWidth << " x " << tmxObject->mHeight
   //      << std::endl;

   _size.x = tmxObject->_width_px;
   _size.y = tmxObject->_height_px;

   _sprite.setPosition(tmxObject->_x_px, tmxObject->_y_px - 24);

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
   auto sizeX = _size.x / PPM;
   auto sizeY = _size.y / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,     0);
   vertices[1] = b2Vec2(0,     sizeY);
   vertices[2] = b2Vec2(sizeX, sizeY);
   vertices[3] = b2Vec2(sizeX, 0);

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

