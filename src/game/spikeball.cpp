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
//           < : O  o : >                circular body (bad spiky ball, dynamic body)
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

}

void SpikeBall::draw(sf::RenderTarget& window)
{
   for (auto i = 0u; i < mChainElements.size() - 1; i++)
   {
      auto c1 = mChainElements[i];
      auto c2 = mChainElements[i + 1];
      const auto c1Pos = c1->GetPosition();
      const auto c2Pos = c2->GetPosition();

      sf::Vertex line[] =
      {
         sf::Vertex(sf::Vector2f(c1Pos.x * PPM, c1Pos.y * PPM)),
         sf::Vertex(sf::Vector2f(c2Pos.x * PPM, c2Pos.y * PPM)),
      };

      window.draw(line, 2, sf::Lines);
   }
}


void SpikeBall::update(const sf::Time& /*dt*/)
{

}


void SpikeBall::setup(const std::shared_ptr<b2World>& world)
{
   // can be removed later
   mGround = world->CreateBody(&mGroundDef);
   mGroundShape.Set(b2Vec2(-40.0f, 0.0f), b2Vec2(40.0f, 0.0f));
   mGround->CreateFixture(&mGroundShape, 0.0f);

   // chain element setup
   mChainElementShape.SetAsBox(0.6f, 0.125f);
   mChainElementFixtureDef.shape = &mChainElementShape;
   mChainElementFixtureDef.density = 20.0f;
   mChainElementFixtureDef.friction = 0.2f;

   mJointDef.collideConnected = false;

   auto prevBody = mGround;
   for (auto i = 0; i < 10; ++i)
   {
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.position.Set(0.5f + i, mChainElementLength);
      auto chainBody = world->CreateBody(&bd);
      chainBody->CreateFixture(&mChainElementFixtureDef);
      mChainElements.push_back(chainBody);

      b2Vec2 anchor(static_cast<float>(i), mChainElementLength);
      mJointDef.Initialize(prevBody, chainBody, anchor);
      world->CreateJoint(&mJointDef);

      prevBody = chainBody;
   }
}

int32_t SpikeBall::getZ() const
{
   return mZ;
}

void SpikeBall::setZ(const int32_t& z)
{
   mZ = z;
}
