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
void Crusher::update(const sf::Time& dt)
{
   updateState();

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
         const auto val = distance_to_be_traveled * Easings::easeOutElastic<float>(mExtractionTime.asSeconds());
         mOffset.y = val;

         mExtractionTime += dt * 0.4f;

         break;
      }
      case State::Retract:
      {
         const auto val = distance_to_be_traveled * (1.0f - Easings::easeInSine<float>(mRetractionTime.asSeconds()));
         mOffset.y = val;

         mRetractionTime += dt * 0.4f;

         if (mInstanceId == 0)
         {
            std::cout << val << std::endl;
         }

         break;
      }
   }

   updateSpritePositions();
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

   setupBody(world);
}


//-----------------------------------------------------------------------------
void Crusher::setupTransform()
{
   auto x = mPixelPosition.x / PPM;
   auto y = mPixelPosition.y / PPM + (5 * PIXELS_PER_TILE) / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void Crusher::setupBody(const std::shared_ptr<b2World>& world)
{
   static constexpr auto BLADE_HORIZONTAL_TILES = 5;
   static constexpr auto BLADE_VERTICAL_TILES = 1;

   constexpr auto sizeX = (BLADE_HORIZONTAL_TILES * PIXELS_PER_TILE) / PPM;
   constexpr auto sizeY = (BLADE_VERTICAL_TILES * PIXELS_PER_TILE) / PPM;

   constexpr auto sharpness = 0.1f;
   constexpr auto tolerance = 0.06f;

   //       +-+
   //       | |
   //       | |
   //       | |
   //       | |
   // +-----+-+------+
   // \             /
   //  \___________/

   b2Vec2 spikeVertices[4];
   spikeVertices[0] = b2Vec2(tolerance, 0);
   spikeVertices[1] = b2Vec2(sharpness + tolerance, sizeY);
   spikeVertices[2] = b2Vec2(sizeX - sharpness - tolerance, sizeY);
   spikeVertices[3] = b2Vec2(sizeX - tolerance, 0);

   b2BodyDef deadlyBodyDef;
   deadlyBodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&deadlyBodyDef);

   setupTransform();

   b2PolygonShape spikeShape;
   spikeShape.Set(spikeVertices, 4);
   auto deadlyFixture = mBody->CreateFixture(&spikeShape, 0);
   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeCrusher);
   deadlyFixture->SetUserData(static_cast<void*>(objectData));

   b2PolygonShape boxShape;
   boxShape.SetAsBox(
      ((PIXELS_PER_TILE * 5) / PPM) * 0.5f,
      (PIXELS_PER_TILE / PPM) * 0.5f,
      {
         ((PIXELS_PER_TILE * 5) / PPM) * 0.5f,
         -(PIXELS_PER_TILE / PPM) * 0.5f
      },
      0.0f
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

