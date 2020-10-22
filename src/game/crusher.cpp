#include "crusher.h"

#include "fixturenode.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


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
}


//-----------------------------------------------------------------------------
void Crusher::draw(sf::RenderTarget& /*target*/)
{

}


//-----------------------------------------------------------------------------
void Crusher::update(const sf::Time& /*dt*/)
{

}


//-----------------------------------------------------------------------------
void Crusher::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& /*world*/)
{
   std::cout << "set up crusher: '" << tmxObject->mName << "'" << std::endl;
}


//-----------------------------------------------------------------------------
void Crusher::setupTransform()
{
   auto x = mPixelPosition.x / PPM - (PIXELS_PER_TILE / (2 * PPM));
   auto y = mPixelPosition.y / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//
//
//       +-+
//       | |
//       | |
//       | |
//       | |
// +-----+-+------+
// \             /
//  \___________/
//


//-----------------------------------------------------------------------------
void Crusher::setupBody(const std::shared_ptr<b2World>& world)
{
   static constexpr auto BLADE_HORIZONTAL_TILES = 5;
   static constexpr auto BLADE_VERTICAL_TILES = 1;

   b2PolygonShape polygonShape;

   auto sizeX = (BLADE_HORIZONTAL_TILES * PIXELS_PER_TILE) / PPM;
   auto sizeY = (BLADE_VERTICAL_TILES * PIXELS_PER_TILE) / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,     0);
   vertices[1] = b2Vec2(0,     sizeY);
   vertices[2] = b2Vec2(sizeX, sizeY);
   vertices[3] = b2Vec2(sizeX, 0);

   polygonShape.Set(vertices, 4);

   b2BodyDef bodyDef;
   bodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&bodyDef);

   setupTransform();

   auto fixture = mBody->CreateFixture(&polygonShape, 0);
   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeMovingPlatform);
   fixture->SetUserData(static_cast<void*>(objectData));
}

// http://www.iforce2d.net/b2dtut/custom-gravity


/*

void LuaNode::makeDynamic()
{
   mBody->SetType(b2_dynamicBody);
}


void LuaNode::makeStatic()
{
   mBody->SetType(b2_staticBody);
}

*/

