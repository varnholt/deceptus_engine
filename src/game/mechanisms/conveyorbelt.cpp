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


std::vector<b2Body*> ConveyorBelt::sBodiesOnBelt;


void ConveyorBelt::setVelocity(float velocity)
{
   mVelocity = velocity;
   mPointsRight = (mVelocity > 0.0f);
}


void ConveyorBelt::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   for (auto& sprite : mBeltSprites)
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
   const auto val = static_cast<int32_t>(mElapsed * 40.0f * fabs(mVelocity)) % BELT_TILE_COUNT;
   const auto xOffset = mPointsRight ? 7 - val :  val;
   auto yOffset = 0u;

   for (auto i = 0u; i < mBeltSprites.size(); i++)
   {
       if (i == 0u)
       {
           // left tile (row 0)
           yOffset = 0;
       }
       else if (i == mBeltSprites.size() - 1)
       {
           // right tile (row 2)
           yOffset = PIXELS_PER_TILE * 2;
       }
       else
       {
           // middle tile (row 1)
           yOffset = PIXELS_PER_TILE;
       }

       mBeltSprites[i].setTextureRect({
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

   mTexture = TexturePool::getInstance().get(basePath / "tilesets" / "cbelt.png");

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
         velocity = velocityIt->second->mValueFloat.value();
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

   mBeltPixelRect.left   = static_cast<int32_t>(x);
   mBeltPixelRect.top    = static_cast<int32_t>(y);
   mBeltPixelRect.height = static_cast<int32_t>(height);
   mBeltPixelRect.width  = static_cast<int32_t>(width);

   static auto ROUND_EPSILON = 0.5f;
   auto tileCount = static_cast<uint32_t>( (width / PIXELS_PER_TILE) + ROUND_EPSILON);
   // std::cout << "estimating " << tileCount << " tiles per belt" << " at " << x << ", " << y << std::endl;

   for (auto i = 0u; i < tileCount; i++)
   {
      sf::Sprite beltSprite;
      beltSprite.setTexture(*mTexture);
      beltSprite.setPosition(
         x + i * PIXELS_PER_TILE,
         y + Y_OFFSET
      );

      mBeltSprites.push_back(beltSprite);
   }

   for (auto i = 0u; i < tileCount - 1; i++)
   {
      sf::Sprite arrowSprite;
      arrowSprite.setTexture(*mTexture);
      arrowSprite.setPosition(
         x + i * PIXELS_PER_TILE + 12,
         y - 12
      );

      arrowSprite.setTextureRect({
           ARROW_INDEX_X * PIXELS_PER_TILE,
           (velocity < -0.0001 ? ARROW_INDEX_LEFT_Y : ARROW_INDEX_RIGHT_Y) * PIXELS_PER_TILE,
           PIXELS_PER_TILE,
           PIXELS_PER_TILE
        }
      );

      mArrowSprites.push_back(arrowSprite);
   }

   updateSprite();

   setZ(ZDepthForegroundMin);
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
   return mBeltPixelRect;
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


