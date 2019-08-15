#include "spikeball.h"

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


void SpikeBall::update(const sf::Time& /*dt*/)
{

}


void SpikeBall::setup(const std::shared_ptr<b2World>& world)
{
   b2Body* m_bodyA;
   b2Body* m_bodyB;
   b2RevoluteJoint* m_joint;

   //create the two bodies
   {
      // body and fixture defs - the common parts
      b2BodyDef bodyDef;
      bodyDef.type = b2_dynamicBody;
      b2FixtureDef fixtureDef;
      fixtureDef.density = 1;

      // two shapes
      b2PolygonShape boxShape;
      boxShape.SetAsBox(2,2);
      b2CircleShape circleShape;
      circleShape.m_radius = 2;

      // make box a little to the left
      bodyDef.position.Set(-13, 10);
      fixtureDef.shape = &boxShape;
      m_bodyA = world->CreateBody( &bodyDef );
      m_bodyA->CreateFixture( &fixtureDef );

      // and circle a little to the right
      bodyDef.position.Set( -7, 10);
      fixtureDef.shape = &circleShape;
      m_bodyB = world->CreateBody( &bodyDef );
      m_bodyB->CreateFixture( &fixtureDef );
   }

   //set up a revolute joint
   {
      b2RevoluteJointDef revoluteJointDef;
      revoluteJointDef.bodyA = m_bodyA;
      revoluteJointDef.bodyB = m_bodyB;
      revoluteJointDef.collideConnected = false;

      // anchor points
      revoluteJointDef.localAnchorA.Set(2,2); //the top right corner of the box
      revoluteJointDef.localAnchorB.Set(0,0); //center of the circle

      // limits
      revoluteJointDef.enableLimit = true;
      revoluteJointDef.lowerAngle = -45 * DEGTORAD;
      revoluteJointDef.upperAngle =  45 * DEGTORAD;

      // motor
      revoluteJointDef.enableMotor = false; //let user toggle this later
      revoluteJointDef.maxMotorTorque = 50;
      revoluteJointDef.motorSpeed = 90 * DEGTORAD; //90 degrees per second

      m_joint = (b2RevoluteJoint*)world->CreateJoint( &revoluteJointDef );
   }

   //make a chain
   {
      // body and fixture defs are common to all chain links
      b2BodyDef bodyDef;
      bodyDef.type = b2_dynamicBody;
      bodyDef.position.Set(5,10);
      b2FixtureDef fixtureDef;
      fixtureDef.density = 1;
      b2PolygonShape polygonShape;
      polygonShape.SetAsBox(1,0.25);
      fixtureDef.shape = &polygonShape;

      // set up the common properties of the joint before entering the loop
      b2RevoluteJointDef revoluteJointDef;
      revoluteJointDef.localAnchorA.Set( 0.75,0);
      revoluteJointDef.localAnchorB.Set(-0.75,0);

      b2Body* link = world->CreateBody( &bodyDef );
      link->CreateFixture( &fixtureDef );

      for (auto i = 0; i < 10; i++)
      {
         b2Body* newLink = world->CreateBody( &bodyDef );
         newLink->CreateFixture( &fixtureDef );

         // inside the loop, only need to change the bodies
         revoluteJointDef.bodyA = link;
         revoluteJointDef.bodyB = newLink;
         world->CreateJoint( &revoluteJointDef );

         //prepare for next iteration
         link = newLink;
      }

      // body with circle fixture
      b2CircleShape circleShape;
      circleShape.m_radius = 2;
      fixtureDef.shape = &circleShape;
      b2Body* chainBase = world->CreateBody( &bodyDef );
      chainBase->CreateFixture( &fixtureDef );

      // a revolute joint to connect the circle to the ground
      // revoluteJointDef.bodyA = m_groundBody;//provided by testbed
      revoluteJointDef.bodyB = chainBase;
      revoluteJointDef.localAnchorA.Set(4,20);//world coords, because m_groundBody is at (0,0)
      revoluteJointDef.localAnchorB.Set(0,0);//center of circle
      world->CreateJoint( &revoluteJointDef );

      // another revolute joint to connect the chain to the circle
      revoluteJointDef.bodyA = link;//the last added link of the chain
      revoluteJointDef.bodyB = chainBase;
      revoluteJointDef.localAnchorA.Set(0.75,0);//the regular position for chain link joints, as above
      revoluteJointDef.localAnchorB.Set(1.75,0);//a little in from the edge of the circle
      world->CreateJoint( &revoluteJointDef );
   }

}
