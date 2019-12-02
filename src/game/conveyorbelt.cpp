#include "conveyorbelt.h"
#include "player.h"
#include "texturepool.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"

#include <iostream>

namespace
{
static const auto Y_OFFSET = -10;
}


std::vector<b2Body*> ConveyorBelt::sBodiesOnBelt;
sf::Texture ConveyorBelt::sTexture;


void ConveyorBelt::setVelocity(float velocity)
{
   mVelocity = velocity;
}


void ConveyorBelt::draw(sf::RenderTarget& target)
{
    for (auto& sprite : mSprites)
    {
       target.draw(sprite);
    }
}


void ConveyorBelt::update(const sf::Time& /*dt*/)
{
}


ConveyorBelt::ConveyorBelt(
   GameNode* parent,
   const std::shared_ptr<b2World>& world,
   TmxObject* tmxObject,
   const std::filesystem::path& basePath
)
 : FixtureNode(parent)
{
   setType(ObjectTypeConveyorBelt);

   if (sTexture.getSize().x == 0)
   {
      sTexture = *TexturePool::getInstance().get(basePath / "tilesets" / "cbelt.png");
   }

   float x      = tmxObject->mX;
   float y      = tmxObject->mY;
   float width  = tmxObject->mWidth;
   float height = tmxObject->mHeight;

   auto velocity = 0.0f;

   if (tmxObject->mProperties)
   {
      auto it = tmxObject->mProperties->mMap.find("velocity");
      if (it != tmxObject->mProperties->mMap.end())
      {
         velocity = it->second->mValueFloat;
      }
   }

   setVelocity(velocity);

   mPositionB2d = b2Vec2(x * MPP, y * MPP);
   mPositionSf.x = x;
   mPositionSf.y = y;

   b2BodyDef bodyDef;
   bodyDef.type = b2_staticBody;
   bodyDef.position = mPositionB2d;
   mBody = world->CreateBody(&bodyDef);

   auto halfPhysicsWidth = width * MPP * 0.5f;
   auto halfPhysicsHeight = height * MPP * 0.5f;

   // create fixture for physical boundaries of the belt object
   mShapeBounds.SetAsBox(
      halfPhysicsWidth, halfPhysicsHeight,
      b2Vec2(halfPhysicsWidth, halfPhysicsHeight),
      0.0f
   );

   b2FixtureDef boundaryFixtureDef;
   boundaryFixtureDef.shape = &mShapeBounds;
   boundaryFixtureDef.density = 1.0f;
   boundaryFixtureDef.isSensor = false;
   auto boundaryFixture = mBody->CreateFixture(&boundaryFixtureDef);
   boundaryFixture->SetUserData(static_cast<void*>(this));

   mPixelRect.left   = static_cast<int32_t>(x);
   mPixelRect.top    = static_cast<int32_t>(y);
   mPixelRect.height = static_cast<int32_t>(height);
   mPixelRect.width  = static_cast<int32_t>(width);

   static auto ROUND_EPSILON = 0.5f;
   auto tileCount = static_cast<uint32_t>( (width / PIXELS_PER_TILE) + ROUND_EPSILON);
   // std::cout << "estimating " << tileCount << " tiles per belt" << " at " << x << ", " << y << std::endl;

   auto yOffset = 0u;
   for (auto i = 0u; i < tileCount; i++)
   {
       if (i == 0u)
       {
           // left tile (row 0)
           yOffset = 0;
       }
       else if (i == tileCount - 1)
       {
           // right tile (row 2)
           yOffset = PIXELS_PER_TILE * 2;
       }
       else
       {
           // middle tile (row 1)
           yOffset = PIXELS_PER_TILE;
       }

       sf::Sprite sprite;
       sprite.setTexture(sTexture);
       sprite.setTextureRect({0, static_cast<int32_t>(yOffset), PIXELS_PER_TILE, PIXELS_PER_TILE});
       sprite.setPosition(x + i * PIXELS_PER_TILE, y + Y_OFFSET);
       mSprites.push_back(sprite);
   }
}


b2Body *ConveyorBelt::getBody() const
{
   return mBody;
}


float ConveyorBelt::getVelocity() const
{
   return mVelocity;
}


void ConveyorBelt::update()
{
   sBodiesOnBelt.clear();
   auto player = Player::getCurrent();
   player->setBeltVelocity(0.0f);
   player->setOnBelt(false);
}


void ConveyorBelt::processFixtureNode(
   FixtureNode* fixtureNode,
   b2Body* collidingBody
)
{
   if (fixtureNode->getType() == ObjectTypeConveyorBelt)
   {
      auto playerBody = Player::getCurrent()->getBody();

      auto belt = dynamic_cast<ConveyorBelt*>(fixtureNode);

      if (!belt->isEnabled())
      {
         return;
      }

      auto beltVelocity = belt->getVelocity();

      // only process a body once since bodies can have multiple fixtures
      if (std::find(sBodiesOnBelt.begin(), sBodiesOnBelt.end(), collidingBody) == sBodiesOnBelt.end())
      {
         auto velocity = collidingBody->GetLinearVelocity();
         velocity.x += beltVelocity;

        if (collidingBody != playerBody)
        {
           collidingBody->SetLinearVelocity(velocity);
           sBodiesOnBelt.push_back(collidingBody);
        }
        else
        {
           // handle player differently because multiple linear velocities are applied to the player
           auto player = Player::getCurrent();
           player->setOnBelt(true);
           player->setBeltVelocity(beltVelocity);
        }
      }
   }
}

sf::IntRect ConveyorBelt::getPixelRect() const
{
   return mPixelRect;
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


