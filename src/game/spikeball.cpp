#include "spikeball.h"

#include "constants.h"

// spike ball concept
//
//                      +-----------+
//                      |           |
//                      |     x     |    box body + rotating revolute joint (static body)
//                      |   ./      |
//                      +-./--------+
//                      ./               thin box body + distance joint      _________   _________   _________
//                    ./                 thin box body + distance joint    -[-o     o-]-[-o     o-]-[-o     o-]-
//                  ./                   thin box body + distance joint     '---------' '---------' '---------'
//            \- __^_ -/
//             .`    '.
//           < : O  x : >                circular body (bad spiky ball, dynamic body)
//             :  __  :
//            /_`----'_\
//                \/
//
// https://www.iforce2d.net/b2dtut/joints-revolute


#ifndef DEGTORAD
#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f
#endif


SpikeBall::SpikeBall(GameNode* node)
 : GameNode(node)
{
   // chain element setup
   mChainElementShape.SetAsBox(0.06f, 0.0125f);
   mChainElementFixtureDef.shape = &mChainElementShape;
   mChainElementFixtureDef.density = 20.0f;
   mChainElementFixtureDef.friction = 0.2f;

   mTexture.loadFromFile("data/sprites/enemy_spikeball.png");
   mSprite.setTexture(mTexture);
   mSprite.setTextureRect(sf::IntRect(24, 0, 48, 48));
   // mSprite.setRotation()
}


void SpikeBall::draw(sf::RenderTarget& window)
{
   static const auto color = sf::Color(255, 0, 0);

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

   if (mBallBody)
   {
      mSprite.setPosition(
         mBallBody->GetPosition().x * PPM - mBallShape.m_radius * PPM,
         mBallBody->GetPosition().y * PPM - mBallShape.m_radius * PPM
      );

      window.draw(mSprite);


      // sf::CircleShape shape(mBallShape.m_radius * PPM);
      // shape.setFillColor(sf::Color(100, 250, 50));
      // shape.setPosition(
      //    mBallBody->GetPosition().x * PPM - mBallShape.m_radius * PPM,
      //    mBallBody->GetPosition().y * PPM - mBallShape.m_radius * PPM
      // );
      //
      // window.draw(shape);
   }
}


void SpikeBall::update(const sf::Time& /*dt*/)
{
}


void SpikeBall::setup(const std::shared_ptr<b2World>& world)
{
   static const auto chainElementLength = 0.3f;
   //static const auto chainElementLength = 0.1f;

   auto pos = b2Vec2{static_cast<float>(mPixelPosition.x * MPP), static_cast<float>(mPixelPosition.y * MPP)};

   // can be removed later
   mGround = world->CreateBody(&mGroundDef);
   mGroundShape.Set(b2Vec2(pos.x - 0.1f, pos.y), b2Vec2(pos.x + 0.1f, pos.y));
   mGround->CreateFixture(&mGroundShape, 0.0f);

   mJointDef.collideConnected = false;

   auto prevBody = mGround;
   for (auto i = 0; i < 10; ++i)
   {
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.position.Set(pos.x + 0.01f + i * chainElementLength, pos.y);
      auto chainBody = world->CreateBody(&bd);
      auto chainFixture = chainBody->CreateFixture(&mChainElementFixtureDef);
      chainFixture->SetSensor(true);
      mChainElements.push_back(chainBody);

      b2Vec2 anchor(pos.x + i * chainElementLength, pos.y);

      //      if (i==0)
      //      {
      //         mJointDef.enableMotor = true;
      //         mJointDef.maxMotorTorque = 5;
      //         mJointDef.motorSpeed = 90 * DEGTORAD;
      //      }
      //      else {
      //         mJointDef.enableMotor = false;
      //         mJointDef.maxMotorTorque = 5;
      //         mJointDef.motorSpeed = 90 * DEGTORAD;
      //      }

      mJointDef.Initialize(prevBody, chainBody, anchor);
      world->CreateJoint(&mJointDef);

      prevBody = chainBody;
   }

   //   revoluteJointDef.enableLimit = true;
   //   revoluteJointDef.lowerAngle = -45 * DEGTORAD;
   //   revoluteJointDef.upperAngle =  45 * DEGTORAD;

   // https://www.iforce2d.net/b2dtut/joints-revolute
   mBallBodyDef.type = b2_dynamicBody;
   mBallFixtureDef.density = 1;
   mBallShape.m_radius = 0.4f;
   mBallBodyDef.position.Set(pos.x + 0.01f + 10 * chainElementLength, pos.y);
   mBallFixtureDef.shape = &mBallShape;
   mBallBody = world->CreateBody( &mBallBodyDef );
   /*auto fixture = */ mBallBody->CreateFixture( &mBallFixtureDef );
   b2Vec2 anchor(pos.x + 10 * chainElementLength, pos.y);
   mJointDef.Initialize(prevBody, mBallBody, anchor);
   world->CreateJoint(&mJointDef);

}

int32_t SpikeBall::getZ() const
{
   return mZ;
}


void SpikeBall::setZ(const int32_t& z)
{
   mZ = z;
}


sf::Vector2i SpikeBall::getPixelPosition() const
{
   return mPixelPosition;
}


void SpikeBall::setPixelPosition(const sf::Vector2i& pixelPosition)
{
   mPixelPosition = pixelPosition;
}


