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
   mPointsRight = (mVelocity > 0.0f);
}


void ConveyorBelt::draw(sf::RenderTarget& target)
{
    for (auto& sprite : mSprites)
    {
       target.draw(sprite);
    }
}


void ConveyorBelt::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (mLeverLag <= 0.0f)
      {
         return;
      }
      else
      {
         mLeverLag -= dt.asSeconds();
      }
   }
   else
   {
      if (mLeverLag < 1.0f)
      {
         mLeverLag += dt.asSeconds();
      }
      else
      {
         mLeverLag = 1.0f;
      }
   }

   mElapsed += mLeverLag * dt.asSeconds();
   updateSprite();
}


void ConveyorBelt::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
   mLeverLag = enabled ? 0.0f : 1.0f;
}


void ConveyorBelt::updateSprite()
{
   const auto val = static_cast<int32_t>(mElapsed * 40.0f * fabs(mVelocity)) % 8;
   const auto xOffset = mPointsRight ? 7 - val :  val;
   auto yOffset = 0u;

   for (auto i = 0u; i < mSprites.size(); i++)
   {
       if (i == 0u)
       {
           // left tile (row 0)
           yOffset = 0;
       }
       else if (i == mSprites.size() - 1)
       {
           // right tile (row 2)
           yOffset = PIXELS_PER_TILE * 2;
       }
       else
       {
           // middle tile (row 1)
           yOffset = PIXELS_PER_TILE;
       }

       mSprites[i].setTextureRect({
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
   TmxObject* tmxObject,
   const std::filesystem::path& basePath
)
 : FixtureNode(parent)
{
   setName(typeid(ConveyorBelt).name());
   setType(ObjectTypeConveyorBelt);

   if (sTexture.getSize().x == 0)
   {
      sTexture = *TexturePool::getInstance().get(basePath / "tilesets" / "cbelt.png");
   }

   float x      = tmxObject->mX;
   float y      = tmxObject->mY;
   float width  = tmxObject->mWidth;
   float height = tmxObject->mHeight;

   auto velocity = mVelocity;

   if (tmxObject->mProperties)
   {
      auto velocityIt = tmxObject->mProperties->mMap.find("velocity");
      if (velocityIt != tmxObject->mProperties->mMap.end())
      {
         velocity = velocityIt->second->mValueFloat;
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

   // auto halfPhysicsWidth = width * MPP * 0.5f;
   // auto halfPhysicsHeight = height * MPP * 0.5f;
   //
   // create fixture for physical boundaries of the belt object
   // mShapeBounds.SetAsBox(
   //    halfPhysicsWidth, halfPhysicsHeight,
   //    b2Vec2(halfPhysicsWidth, halfPhysicsHeight),
   //    0.0f
   // );

   const auto pWidth = width * MPP;
   const auto pHeight = height * MPP;

   constexpr auto dx = 0.002f;
   constexpr auto dy = 0.001f;
   std::array<b2Vec2, 6> vertices {
      b2Vec2{dx,           0.0},
      b2Vec2{0.0,          pHeight - dy},
      b2Vec2{0.0,          pHeight},
      b2Vec2{pWidth,       pHeight},
      b2Vec2{pWidth,       pHeight - dy},
      b2Vec2{pWidth - dx,  0.0}
   };

   mShapeBounds.Set(vertices.data(), static_cast<int32_t>(vertices.size()));

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

   for (auto i = 0u; i < tileCount; i++)
   {
      sf::Sprite sprite;
      sprite.setTexture(sTexture);
      sprite.setPosition(x + i * PIXELS_PER_TILE, y + Y_OFFSET);
      mSprites.push_back(sprite);
   }

   updateSprite();
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


