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

   mTexture = TexturePool::getInstance().get("data/level-malte/tilesets/crypts.png");
   mSprite.setTexture(*mTexture.get());
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(mSprite);
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{
   const auto x = mBody->GetPosition().x * PPM;
   const auto y = mBody->GetPosition().y * PPM;
   mSprite.setPosition(x, y - 24);
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

   mSize.x = tmxObject->_width_px;
   mSize.y = tmxObject->_height_px;

   mSprite.setPosition(tmxObject->_x_px, tmxObject->_y_px - 24);

   switch (static_cast<int32_t>(mSize.x))
   {
      case 24:
      {
         mSprite.setTextureRect(sf::IntRect(1392, 0, 24, 2 * 24));
         break;
      }

      case 48:
      {
         mSprite.setTextureRect(sf::IntRect(1296, 24, 2 * 24, 3 * 24));
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
   auto x = mSprite.getPosition().x / PPM;
   auto y = mSprite.getPosition().y / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//--------------------------------------------------------------------------------------------------
void MoveableBox::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;
   auto sizeX = mSize.x / PPM;
   auto sizeY = mSize.y / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,     0);
   vertices[1] = b2Vec2(0,     sizeY);
   vertices[2] = b2Vec2(sizeX, sizeY);
   vertices[3] = b2Vec2(sizeX, 0);

   polygonShape.Set(vertices, 4);

   b2BodyDef bodyDef;
   bodyDef.type = b2_dynamicBody;
   mBody = world->CreateBody(&bodyDef);
   // mBody->SetGravityScale(0.0f);

   setupTransform();

   auto fixture = mBody->CreateFixture(&polygonShape, 0);
   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeMoveableBox);
   fixture->SetUserData(static_cast<void*>(objectData));
}

