#include "bouncer.h"
#include "fixturenode.h"
#include "globalclock.h"
#include "player.h"

#include <iostream>

const auto SPRITE_WIDTH = 24;
const auto SPRITE_HEIGHT = 24;

//
//        +--------------------------------------------------+ <-
//        |                                                  | sensor surface
//        +--------------------------------------------------+ <-
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        +--------------------------------------------------+
//


b2Body *Bouncer::getBody() const
{
  return mBody;
}

int Bouncer::getZ() const
{
  return mZ;
}

void Bouncer::setZ(int z)
{
  mZ = z;
}

Bouncer::Bouncer(
   GameNode* parent,
   const std::shared_ptr<b2World>& world,
   float x,
   float y,
   float width,
   float height
)
 : FixtureNode(parent)
{
  // std::cout << "creating bouncer at " << x << ", " << y << " (" << width << " x " << height << ")" << std::endl;

  setType(ObjectTypeBouncer);
  mActivationTime = GlobalClock::getInstance()->getElapsedTime();

  mPositionB2d = b2Vec2(x * MPP, y * MPP);
  mPositionSf.x = x;
  mPositionSf.y = y + height;

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

  mBody->CreateFixture(&boundaryFixtureDef);

  // create fixture for the sensor behavior, collision notification
  mShapeBounds.SetAsBox(
    halfPhysicsWidth, halfPhysicsHeight,
    b2Vec2(halfPhysicsWidth, halfPhysicsHeight),
    0.0f
  );

  b2FixtureDef sensorFixtureDef;
  sensorFixtureDef.shape = &mShapeBounds;
  sensorFixtureDef.isSensor = true;

  auto fixture = mBody->CreateFixture(&sensorFixtureDef);
  fixture->SetUserData((void*)this);

  // load texture
  if (mTexture.loadFromFile("data/level-crypt/tilesets/bumper.png"))
  {
     mSprite.setTexture(mTexture);
  }

  mSprite.setPosition(
    mPositionSf - sf::Vector2f(0.0f, static_cast<float>(SPRITE_HEIGHT))
  );
}


void Bouncer::draw(sf::RenderTarget &window)
{
  window.draw(mSprite);
}


void Bouncer::update(float /*dt*/)
{
  auto now = GlobalClock::getInstance()->getElapsedTime();
  auto delta = (now - mActivationTime).asMilliseconds();

  int step = static_cast<int>(delta * 0.02f);
  if (step > 9)
  {
    step = 0;
  }

  // printf("step: %d\n", step);

  mSprite.setTextureRect(
     sf::IntRect(
        step * SPRITE_WIDTH,
        0,
        SPRITE_WIDTH,
        SPRITE_HEIGHT
     )
  );
}


Bouncer::~Bouncer()
{
  mBody->GetWorld()->DestroyBody(mBody);
}


void Bouncer::activate()
{
  auto now = GlobalClock::getInstance()->getElapsedTime();
  auto delta = (now - mActivationTime).asSeconds();

  if (delta < 0.3f) // set to 0.5?
  {
    // printf("hop, ignore\n");
    return;
  }

  // printf("hop %f\n", delta);

  mActivationTime = now;

  auto forceValue = 0.6f;

  b2Vec2 force;
  switch (mAlignment)
  {
    case PointsUp:
      force = b2Vec2{0.0f, -forceValue};
      break;
    case PointsDown:
      force = b2Vec2{0.0f, forceValue};
      break;
    case PointsLeft:
      force = b2Vec2{-forceValue, 0};
      break;
    case PointsRight:
      force = b2Vec2{forceValue, 0};
      break;
    case PointsNowhere:
    break;
  }

  auto body = Player::getPlayer(0)->getBody();

  // it's pretty important to reset the body's y velocity
  const auto& velocity = body->GetLinearVelocity();
  body->SetLinearVelocity(b2Vec2(velocity.x, 0.0f));

  // aaaaand.. up!
  const auto& pos = body->GetWorldCenter();
  body->ApplyLinearImpulse(force, pos, true);
}


