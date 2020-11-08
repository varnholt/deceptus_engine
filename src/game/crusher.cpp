#include "crusher.h"

#include "fixturenode.h"

#include "easings/easings.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>

sf::Texture Crusher::mTexture;
int32_t Crusher::mInstanceCounter = 0;

static constexpr auto BLADE_HORIZONTAL_TILES = 5;
static constexpr auto BLADE_VERTICAL_TILES = 1;

static constexpr auto BLADE_SIZE_X = (BLADE_HORIZONTAL_TILES * PIXELS_PER_TILE) / PPM;
static constexpr auto BLADE_SIZE_Y = (BLADE_VERTICAL_TILES * PIXELS_PER_TILE) / PPM;

static constexpr auto BLADE_SHARPNESS = 0.1f;
static constexpr auto BLADE_TOLERANCE = 0.06f;


// 7 x 5
//
//         0123456 789ABC
//
//         <#          #>      0
//         <#          #>      1
//         <#####  #####>      2
//         <#          #>      3
//         <#          #>      4
//
//         MMMMM      #        5
//         #####      #        6
//           #        #        7
//           #        #        8
//           #      #####      9
//           #      VVVVV      A
//
//         0123456 0123456     B
//         0123456 0123456     C
//         0123456 0123456     D
//         0123456 0123456     E

//-----------------------------------------------------------------------------
Crusher::Crusher(GameNode* parent)
   : GameNode(parent)
{
   setName("DeathBlock");

   mInstanceId = mInstanceCounter;
   mInstanceCounter++;
}


//-----------------------------------------------------------------------------
void Crusher::draw(sf::RenderTarget& target)
{
   target.draw(mSpriteSpike);
   target.draw(mSpritePusher);
   target.draw(mSpriteMount);
}


//-----------------------------------------------------------------------------
void Crusher::step(const sf::Time& dt)
{
   const auto distance_to_be_traveled = 48.0f;

   switch (mState)
   {
      case State::Idle:
      {
         mIdleTime += dt;
         break;
      }
      case State::Extract:
      {
         const auto val = distance_to_be_traveled * Easings::easeOutBounce<float>(mExtractionTime.asSeconds());
         mOffset.y = val;

         mExtractionTime += dt * 1.0f;

         break;
      }
      case State::Retract:
      {
         const auto val = distance_to_be_traveled * (1.0f - Easings::easeInSine<float>(mRetractionTime.asSeconds()));
         mOffset.y = val;

         mRetractionTime += dt * 0.4f;

         break;
      }
   }
}


void Crusher::update(const sf::Time& dt)
{
   updateState();
   step(dt);
   updateSpritePositions();
   updateTransform();
}


//-----------------------------------------------------------------------------
void Crusher::updateState()
{
   switch (mMode)
   {
      case Mode::Interval:
      {
         switch (mState)
         {
            case State::Idle:
            {
               // go to extract when idle time is elapsed
               if (mIdleTime.asSeconds() > 3.0f)
               {
                  mIdleTime = {};

                  if (mPreviousState == State::Retract || mPreviousState == State::Idle)
                  {
                     mState = State::Extract;
                  }
                  else
                  {
                     mState = State::Retract;
                  }
               }
               break;
            }
            case State::Extract:
            {
               // extract until normalised extraction time is 1
               if (mExtractionTime.asSeconds() >= 1.0f)
               {
                  mState = State::Idle;
                  mPreviousState = State::Extract;
                  mExtractionTime = {};
               }
               break;
            }
            case State::Retract:
            {
               // retract until normalised retraction time is 1
               if (mRetractionTime.asSeconds() >= 1.0f)
               {
                  mState = State::Idle;
                  mPreviousState = State::Retract;
                  mRetractionTime = {};
               }
               break;
            }
         }

         break;
      }
      case Mode::Distance:
      {
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Crusher::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   if (mTexture.getSize().x == 0)
   {
      mTexture.loadFromFile("data/level-crypt/tilesets/crushers.png");
   }

   // crusher down
   //
   //    A        5
   //    A        6
   //    B        7
   //    C        8
   //  #####      9
   //  VVVVV      A
   //
   // A: 216, 122 -> 240, 168 -> mount, 2 tiles offset from left, 16px offset from top
   // B: 216, 168 -> 240, 192 -> pusher
   // C: 168, 192 -> 288, 264 -> spikes, 2 tiles offset from left

   // std::cout << "set up crusher: '" << tmxObject->mName << "'" << std::endl;

   mPixelPosition.x = tmxObject->mX;
   mPixelPosition.y = tmxObject->mY;

   mSpriteMount.setTexture(mTexture);
   mSpritePusher.setTexture(mTexture);
   mSpriteSpike.setTexture(mTexture);

   mSpriteMount.setTextureRect({
         7 * PIXELS_PER_TILE,
         5 * PIXELS_PER_TILE,
         5 * PIXELS_PER_TILE,
         2 * PIXELS_PER_TILE
      }
   );

   mSpritePusher.setTextureRect({
         7 * PIXELS_PER_TILE,
         7 * PIXELS_PER_TILE,
         5 * PIXELS_PER_TILE,
         1 // * PIXELS_PER_TILE - i only want this to be one pixel in height so scaling is easy
      }
   );

   mSpriteSpike.setTextureRect({
         7 * PIXELS_PER_TILE,
         8 * PIXELS_PER_TILE,
         5 * PIXELS_PER_TILE,
         3 * PIXELS_PER_TILE
      }
   );

   // alignment:
   //    down
   //    up
   //    right
   //    left

   if (tmxObject->mProperties)
   {
      const auto it = tmxObject->mProperties->mMap.find("alignment");

      if (it != tmxObject->mProperties->mMap.end())
      {
         // std::cout << it->second->mValueStr << std::endl;

         const auto alignment = it->second->mValueStr;
         if (alignment == "up")
         {
            mAlignment = Alignment::PointsUp;
         }
         else if (alignment == "down")
         {
            mAlignment = Alignment::PointsDown;
         }
         else if (alignment == "left")
         {
            mAlignment = Alignment::PointsLeft;
         }
         else if (alignment == "right")
         {
            mAlignment = Alignment::PointsRight;
         }
      }
   }

   setupBody(world);
}


//-----------------------------------------------------------------------------
void Crusher::updateTransform()
{
   auto x = mPixelPosition.x / PPM;
   auto y = (mOffset.y + mPixelPosition.y - PIXELS_PER_TILE) / PPM + (5 * PIXELS_PER_TILE) / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0.0f);
}


//-----------------------------------------------------------------------------
void Crusher::setupBody(const std::shared_ptr<b2World>& world)
{
   switch (mAlignment)
   {
      case Alignment::PointsUp:
      {
         mAngle = 0;
         mOffsetBox.y = PIXELS_PER_TILE / PPM;
         mOffsetBlade.y = -PIXELS_PER_TILE / PPM;
         break;
      }
      case Alignment::PointsLeft:
      {
         mAngle = static_cast<float>(M_PI / 2.0f);
         mOffsetBlade.x = PIXELS_PER_TILE / PPM;
         break;
      }
      case Alignment::PointsRight:
      {
         mAngle = static_cast<float>(-M_PI / 2.0f);
         mOffsetBlade.x = -PIXELS_PER_TILE / PPM;
         break;
      }
      case Alignment::PointsNowhere:
      case Alignment::PointsDown:
      {
         mAngle = static_cast<float>(M_PI);
         break;
      }
   }

   //       +-+
   //       | |
   //       | |
   //       | |
   //       | |
   // +-----+-+------+
   // \             /
   //  \___________/

   b2Vec2 bladeVertices[4];
   bladeVertices[0] = b2Vec2(                                 BLADE_TOLERANCE, 0);
   bladeVertices[1] = b2Vec2(               BLADE_SHARPNESS + BLADE_TOLERANCE, BLADE_SIZE_Y);
   bladeVertices[2] = b2Vec2(BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE, BLADE_SIZE_Y);
   bladeVertices[3] = b2Vec2(BLADE_SIZE_X                   - BLADE_TOLERANCE, 0);

   // manually rotate the blade vertices according to the angle
   b2Vec2 origin{BLADE_SIZE_X / 2.0f, BLADE_SIZE_Y / 2.0f};

   for (auto i = 0; i < 4; i++)
   {
      bladeVertices[i] -= mOffsetBlade;

      const auto x = bladeVertices[i].x;
      const auto y = bladeVertices[i].y;

      bladeVertices[i].x = ((x - origin.x) * cos(mAngle)) - ((origin.y - y) * sin(mAngle)) + origin.x;
      bladeVertices[i].y = ((origin.y - y) * cos(mAngle)) - ((x - origin.x) * sin(mAngle)) + origin.y;

      // bladeVertices[i] += mOffsetBlade;
   }

   b2BodyDef deadlyBodyDef;
   deadlyBodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&deadlyBodyDef);

   b2PolygonShape spikeShape;
   spikeShape.Set(bladeVertices, 4);
   auto deadlyFixture = mBody->CreateFixture(&spikeShape, 0);

   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeCrusher);
   deadlyFixture->SetUserData(static_cast<void*>(objectData));

   b2PolygonShape boxShape;
   boxShape.SetAsBox(
      ((PIXELS_PER_TILE * 5) / PPM) * 0.5f,     // hx
      (PIXELS_PER_TILE / PPM) * 0.5f,           // hy
      {                                         // center
         ((PIXELS_PER_TILE * 5) / PPM) * 0.5f + mOffsetBox.x,
         -(PIXELS_PER_TILE / PPM)      * 0.5f + mOffsetBox.y
      },
      mAngle
   );

   mBody->CreateFixture(&boxShape, 0);
}


void Crusher::updateSpritePositions()
{
   // the mount stays where it is
   mSpriteMount.setPosition(
      sf::Vector2f{
         mPixelPosition.x,
         mPixelPosition.y
      }
   );

   mSpritePusher.setScale(1.0f, mOffset.y);

   mSpritePusher.setPosition(
      sf::Vector2f{
         mPixelPosition.x,
         mPixelPosition.y + 2 * PIXELS_PER_TILE
      }
   );

   auto pixelPosition = mPixelPosition;
   pixelPosition += mOffset;

   mSpriteSpike.setPosition(
      sf::Vector2f{
         pixelPosition.x,
         pixelPosition.y + 2 * PIXELS_PER_TILE
      }
   );

}
