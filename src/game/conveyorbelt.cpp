#include "conveyorbelt.h"
#include "player.h"


std::vector<b2Body*> ConveyorBelt::sBodiesOnBelt;


// are belts always on?
// are there going to be switches to turn them on or off?
// in case they're always on, we can just animate them in the tmx

void ConveyorBelt::setVelocity(float velocity)
{
  mVelocity = velocity;
}

ConveyorBelt::ConveyorBelt(
   GameNode* parent,
   const std::shared_ptr<b2World>& world,
   float x,
   float y,
   float width,
   float height
)
 : FixtureNode(parent)
{
  setType(ObjectTypeConveyorBelt);

  mPositionB2d = b2Vec2(x * MPP, y * MPP);
  mPositionSf.x = x;
  mPositionSf.y = y;

  b2BodyDef bodyDef;
  bodyDef.type = b2_staticBody;
  bodyDef.position = mPositionB2d;
  mBody = world->CreateBody(&bodyDef);

  auto halfPhysicsWidth = width * MPP * 0.5f;
  auto halfPhysicsHeight = height * MPP * 0.5f;

  // create fixture for physical boundaries of the bouncer object
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
  boundaryFixture->SetUserData((void*)this);

  // create fixture for the sensor behavior, collision notification
  mShapeBounds.SetAsBox(
    halfPhysicsWidth, halfPhysicsHeight,
    b2Vec2(halfPhysicsWidth, halfPhysicsHeight),
    0.0f
  );

//  b2FixtureDef sensorFixtureDef;
//  sensorFixtureDef.shape = &mShapeBounds;
//  sensorFixtureDef.isSensor = true;
//  auto fixture = mBody->CreateFixture(&sensorFixtureDef);
//  fixture->SetUserData((void*)this);
}


b2Body *ConveyorBelt::getBody() const
{
  return mBody;
}


int ConveyorBelt::getZ() const
{
  return mZ;
}


void ConveyorBelt::setZ(int z)
{
  mZ = z;
}


float ConveyorBelt::getVelocity() const
{
  return mVelocity;
}


void ConveyorBelt::update()
{
  sBodiesOnBelt.clear();
  auto player = Player::getPlayer(0);
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
    auto playerBody = Player::getPlayer(0)->getBody();

    auto beltVelocity = dynamic_cast<ConveyorBelt*>(fixtureNode)->getVelocity();

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
        auto player = Player::getPlayer(0);
        player->setOnBelt(true);
        player->setBeltVelocity(beltVelocity);
      }
    }
  }
}


void ConveyorBelt::processContact(b2Contact* contact)
{
  auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
  auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

  if (fixtureUserDataA)
  {
     auto fixtureNode = (FixtureNode*)fixtureUserDataA;
     processFixtureNode(fixtureNode, contact->GetFixtureB()->GetBody());
  }

  if (fixtureUserDataB)
  {
     auto fixtureNode = (FixtureNode*)fixtureUserDataB;
     processFixtureNode(fixtureNode, contact->GetFixtureA()->GetBody());
  }
}


