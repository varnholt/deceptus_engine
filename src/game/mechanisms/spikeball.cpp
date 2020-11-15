#include "spikeball.h"

#include "constants.h"
#include "fixturenode.h"
#include "hermitecurve.h"
#include "player/player.h"
#include "tmxparser/tmxobject.h"

/*
   spike ball concept

                        +-----------+
                        |           |
                        |     x     |    box body + rotating revolute joint (static body)
                        |   ./      |
                        +-./--------+
                        ./               thin box body + distance joint      _________   _________   _________
                      ./                 thin box body + distance joint    -[-o     o-]-[-o     o-]-[-o     o-]-
                    ./                   thin box body + distance joint     '---------' '---------' '---------'
              \- __^_ -/
               .`    '.
             < : O  x : >                circular body (bad spiky ball, dynamic body)
               :  __  :
              /_`----'_\
                  \/

   https://www.iforce2d.net/b2dtut/joints-revolute
*/


SpikeBall::SpikeBall(GameNode* node)
 : GameNode(node)
{
   setZ(16);

   // chain element setup
   mChainElementShape.SetAsBox(mConfig.chainElementWidth, mConfig.chainElementHeight);
   mChainElementFixtureDef.shape = &mChainElementShape;
   mChainElementFixtureDef.density = 20.0f;
   mChainElementFixtureDef.friction = 0.2f;

   mTexture.loadFromFile("data/sprites/enemy_spikeball.png");
   mSpikeSprite.setTexture(mTexture);
   mSpikeSprite.setTextureRect(sf::IntRect(24, 0, 48, 48));
   mSpikeSprite.setOrigin(24, 24);

   mBoxSprite.setTexture(mTexture);
   mBoxSprite.setTextureRect(sf::IntRect(72, 45, 24, 27));
   mBoxSprite.setOrigin(12, 15);

   mChainElementA.setTexture(mTexture);
   mChainElementA.setTextureRect(sf::IntRect(0, 64, 8, 8));
   mChainElementA.setOrigin(4, 4);

   mChainElementB.setTexture(mTexture);
   mChainElementB.setTextureRect(sf::IntRect(34, 64, 8, 8));
   mChainElementB.setOrigin(4, 4);
}


void SpikeBall::drawSpline(sf::RenderTarget& window)
{
   std::vector<HermiteCurveKey> keys;

   auto t = 0.0f;
   auto ti = 1.0f / mChainElements.size();
   for (auto c : mChainElements)
   {
      HermiteCurveKey k;
      k.mPosition = sf::Vector2f{c->GetPosition().x * PPM, c->GetPosition().y * PPM};
      k.mTime = (t += ti);
      keys.push_back(k);
   }

   HermiteCurve curve;
   curve.setPositionKeys(keys);
   curve.compute();

   auto val = 0.0f;
   auto increment = 1.0f / mConfig.splinePointCount;
   for (auto i = 0; i < mConfig.splinePointCount; i++)
   {
      auto point = curve.computePoint(val += increment);

      auto& element = (i % 2 == 0) ? mChainElementA : mChainElementB;
      element.setPosition(point);

      window.draw(element);
   }
}


void SpikeBall::draw(sf::RenderTarget& window)
{
   static const auto color = sf::Color(200, 200, 240);
   static const bool drawDebugLine = false;

   if (drawDebugLine)
   {
      for (auto i = 0u; i < mChainElements.size() - 1; i++)
      {
         auto c1 = mChainElements[i];
         auto c2 = mChainElements[i + 1];
         const auto c1Pos = c1->GetPosition();
         const auto c2Pos = c2->GetPosition();

         sf::Vertex line[] =
         {
            sf::Vertex(sf::Vector2f(c1Pos.x * PPM, c1Pos.y * PPM), color),
            sf::Vertex(sf::Vector2f(c2Pos.x * PPM, c2Pos.y * PPM), color),
         };

         window.draw(line, 2, sf::Lines);
         // printf("draw %d: %f, %f -> %f, %f\n", i, c1Pos.x * PPM, c1Pos.y * PPM, c2Pos.x * PPM, c2Pos.y * PPM);
      }
   }

   window.draw(mBoxSprite);
   drawSpline(window);
   window.draw(mSpikeSprite);
}


void SpikeBall::update(const sf::Time& dt)
{
   mSpikeSprite.setPosition(
      mBallBody->GetPosition().x * PPM,
      mBallBody->GetPosition().y * PPM
   );

   static const b2Vec2 up{0.0, 1.0};

   auto c1 = mChainElements[0]->GetPosition();
   auto c2 = mChainElements[static_cast<size_t>(mConfig.chainElementCount - 1)]->GetPosition();

   auto c = (c2 - c1);
   c.Normalize();

   // mAngle = acos(b2Dot(up, c) / (c.Length() * up.Length()));
   mAngle = acos(b2Dot(up, c) / (c.LengthSquared() * up.LengthSquared()));

   if (c.x > 0.0f)
   {
      mAngle = -mAngle;
   }

   mSpikeSprite.setRotation(mAngle * RADTODEG);

   // slightly push the ball all the way while it's moving from the right to the left
   auto f = dt.asSeconds() * mConfig.pushFactor;
   if (mBallBody->GetLinearVelocity().x < 0.0f)
   {
      mBallBody->ApplyLinearImpulse(b2Vec2{-f, f}, mBallBody->GetWorldCenter(), true);
   }
}


void SpikeBall::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
    setPixelPosition(
       sf::Vector2i{
          static_cast<int32_t>(tmxObject->mX),
          static_cast<int32_t>(tmxObject->mY)
       }
    );

   auto pos = b2Vec2{static_cast<float>(mPixelPosition.x * MPP), static_cast<float>(mPixelPosition.y * MPP)};

   // can be removed later
   mGround = world->CreateBody(&mGroundDef);
   mGroundShape.Set(b2Vec2(pos.x - 0.1f, pos.y), b2Vec2(pos.x + 0.1f, pos.y));
   mGround->CreateFixture(&mGroundShape, 0.0f);

   mJointDef.collideConnected = false;

   auto prevBody = mGround;
   for (auto i = 0; i < mConfig.chainElementCount; ++i)
   {
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.position.Set(pos.x + 0.01f + i * mConfig.chainElementDistance, pos.y);
      auto chainBody = world->CreateBody(&bd);
      auto chainFixture = chainBody->CreateFixture(&mChainElementFixtureDef);
      chainFixture->SetSensor(true);
      mChainElements.push_back(chainBody);

      b2Vec2 anchor(pos.x + i * mConfig.chainElementDistance, pos.y);

      mJointDef.Initialize(prevBody, chainBody, anchor);
      world->CreateJoint(&mJointDef);

      prevBody = chainBody;
   }

   // attach the spiky ball to the last chain element
   mBallBodyDef.type = b2_dynamicBody;
   mBallFixtureDef.density = 1;
   mBallShape.m_radius = 0.45f;
   mBallBodyDef.position.Set(pos.x + 0.01f + mConfig.chainElementCount * mConfig.chainElementDistance, pos.y);
   mBallFixtureDef.shape = &mBallShape;
   mBallBody = world->CreateBody( &mBallBodyDef );
   auto ballFixture = mBallBody->CreateFixture( &mBallFixtureDef );
   b2Vec2 anchor(pos.x + mConfig.chainElementCount * mConfig.chainElementDistance, pos.y);
   mJointDef.Initialize(prevBody, mBallBody, anchor);
   world->CreateJoint(&mJointDef);

   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeDeadly);
   ballFixture->SetUserData(static_cast<void*>(objectData));


   // that box only needs to be set up once
   mBoxSprite.setPosition(
      mChainElements[0]->GetPosition().x * PPM,
      mChainElements[0]->GetPosition().y * PPM
   );
}


sf::Vector2i SpikeBall::getPixelPosition() const
{
   return mPixelPosition;
}


void SpikeBall::setPixelPosition(const sf::Vector2i& pixelPosition)
{
   mPixelPosition = pixelPosition;
}


