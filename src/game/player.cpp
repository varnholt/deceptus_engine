#include "player.h"

#include <SFML/Graphics.hpp>

#include "audio.h"
#include "displaymode.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "globalclock.h"
#include "fixturenode.h"
#include "joystick/gamecontroller.h"
#include "level.h"
#include "animationpool.h"
#include "physicsconfiguration.h"
#include "weapon.h"

#include <iostream>


std::vector<Player*> Player::sPlayerList;
int Player::sNextId = 0;


//----------------------------------------------------------------------------------------------------------------------
b2Body* Player::getBody() const
{
    return mBody;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getBeltVelocity() const
{
  return mBeltVelocity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setBeltVelocity(float beltVelocity)
{
  mBeltVelocity = beltVelocity;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnBelt() const
{
  return mIsOnBelt;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setOnBelt(bool onBelt)
{
  mIsOnBelt = onBelt;
}


//----------------------------------------------------------------------------------------------------------------------
Player::Player(GameNode* parent)
  : GameNode(parent),
    mId(sNextId++)
{
   sPlayerList.push_back(this);

   mExtraManager = std::make_shared<ExtraManager>();
   mWeapon = new Weapon();
   mExtraTable = std::make_shared<ExtraTable>();
}


//----------------------------------------------------------------------------------------------------------------------
Player::~Player()
{
}


//----------------------------------------------------------------------------------------------------------------------
void Player::initialize()
{
  setBodyViaPixelPosition(
     Level::getCurrentLevel()->getStartPosition().x,
     Level::getCurrentLevel()->getStartPosition().y
  );

  mSpriteAnim.x = PLAYER_TILES_WIDTH;
  mSpriteAnim.y = 0;

  mPortalClock.restart();
  mDamageClock.restart();

  createPlayerBody();

  mJumpDustLeftAligned  = AnimationPool::getInstance().add("player_jump_dust_left_aligned");
  mJumpDustRightAligned = AnimationPool::getInstance().add("player_jump_dust_right_aligned");
  mIdleRightAligned     = AnimationPool::getInstance().add("player_idle_right_aligned");
  mIdleLeftAligned      = AnimationPool::getInstance().add("player_idle_left_aligned");
  mRunRightAligned      = AnimationPool::getInstance().add("player_run_right_aligned");
  mRunLeftAligned       = AnimationPool::getInstance().add("player_run_left_aligned");
  mDashRightAligned     = AnimationPool::getInstance().add("player_dash_right_aligned");
  mDashLeftAligned      = AnimationPool::getInstance().add("player_dash_left_aligned");
  mCrouchRightAligned   = AnimationPool::getInstance().add("player_crouch_right_aligned");
  mCrouchLeftAligned    = AnimationPool::getInstance().add("player_crouch_left_aligned");

  if (mTexture.loadFromFile("data/sprites/player.png"))
  {
     // mSprite.scale(4.0f, 4.0f);
     mSprite.setTexture(mTexture);
  }
  else
  {
     printf("failed loading player spriteset");
  }

  initializeController();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::initializeController()
{
   if (GameControllerIntegration::getCount() > 0)
   {
      auto gji = GameControllerIntegration::getInstance(0);

      gji->getController()->addButtonPressedCallback(
        SDL_CONTROLLER_BUTTON_A,
        [this](){
            auto mode = GameState::getInstance().getMode();
            if (mode == ExecutionMode::Running)
            {
               jump();
            }
         }
      );

      gji->getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, [this](){updateDash(Dash::Left);});
      gji->getController()->addButtonReleasedCallback(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, [this](){updateDash(Dash::Right);});
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<ExtraManager> Player::getExtraManager() const
{
  return mExtraManager;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setBodyViaPixelPosition(float x, float y)
{
   setPixelPosition(x, y);

   if (mBody)
   {
      mBody->SetTransform(
         b2Vec2(x * MPP, y * MPP),
         0.0f
      );
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::draw(sf::RenderTarget& target)
{
   if (!mVisible)
   {
      return;
   }

   auto time = GlobalClock::getInstance()->getElapsedTimeInMs();
   auto damageTime = mDamageClock.getElapsedTime().asMilliseconds();
   if (time > 3000 && damageTime < 3000)
   {
      if ((damageTime / 100) % 2 == 0)
      {
         return;
      }
   }

   updateAnimationOffset();

   mSprite.setTextureRect(
      sf::IntRect(
         mSpriteAnim.x * PLAYER_TILES_WIDTH,
         mSpriteAnim.y,
         PLAYER_TILES_WIDTH,
         PLAYER_TILES_HEIGHT
      )
   );

   mSprite.setPosition(
        mPixelPosition
      - sf::Vector2f(
         PLAYER_TILES_WIDTH / 2,
         PLAYER_ACTUAL_HEIGHT + 8 // WHERE DOES THIS OFFSET COME FROM??
      )
   );

   target.draw(mSprite);

   const auto& animations = AnimationPool::getInstance().getAnimations();
   for (auto it = animations.begin(); it != animations.end(); ++it)
   {
      target.draw(*(*it));
   }
}


//----------------------------------------------------------------------------------------------------------------------
sf::Vector2f Player::getPixelPosition() const
{
   return mPixelPosition;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPixelPosition(float x, float y)
{
   mPixelPosition.x = x;
   mPixelPosition.y = y;
}


//----------------------------------------------------------------------------------------------------------------------
sf::Rect<int> Player::getPlayerRect() const
{
   sf::Rect<int> rect;

   rect.left = static_cast<int>(mPixelPosition.x);
   rect.top = static_cast<int>(mPixelPosition.y);
   rect.width = PLAYER_TILES_WIDTH;
   rect.height = PLAYER_TILES_HEIGHT;

   return rect;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setHeadEnabled(bool enabled)
{
   b2Filter filter;
   filter.maskBits = enabled ? 0xFFFF : 2;
   mHeadFixture->SetFilterData(filter);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createHead()
{
   b2FixtureDef fixtureDefHead;
   fixtureDefHead.density = 1.f;
   fixtureDefHead.friction = PhysicsConfiguration::getInstance().mPlayerFriction;
   fixtureDefHead.restitution = 0.0f;
   fixtureDefHead.filter.groupIndex = -1;

   b2CircleShape headShape;
   headShape.m_p.Set(0, -14 / PPM);
   headShape.m_radius = 0.16f;
   fixtureDefHead.shape = &headShape;

   mHeadFixture = mBody->CreateFixture(&fixtureDefHead);

   FixtureNode* objectDataHead = new FixtureNode(this);
   objectDataHead->setType(ObjectTypePlayer);
   objectDataHead->setFlag("head", true);
   mHeadFixture->SetUserData(static_cast<void*>(objectDataHead));
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createFeet()
{
   auto width  = PLAYER_ACTUAL_WIDTH;
   auto height = PLAYER_ACTUAL_HEIGHT;

   // feet
   //  (   )  (   )  (   )  (   )
   //      ____      _____
   //      ^  ^      ^   ^
   //      dist      radius * 2
   //  __________________________
   //  ^                        ^
   //  count * (dist + radius)

   int feetCount = 4;
   float feetRadius = 0.16f / static_cast<float>(feetCount);
   float feetDist = 0.0f;
   float feetOffset = static_cast<float>(feetCount) * (feetRadius * 2.0f + feetDist) * 0.5f - feetRadius;

   for (int i = 0; i < feetCount; i++)
   {
      b2FixtureDef fixtureDefFeet;
      fixtureDefFeet.density = 1.f;
      fixtureDefFeet.friction = PhysicsConfiguration::getInstance().mPlayerFriction;
      fixtureDefFeet.restitution = 0.0f;
      fixtureDefFeet.filter.groupIndex = -1;
      b2CircleShape feetShape;
      feetShape.m_p.Set(i * (feetRadius * 2.0f + feetDist) - feetOffset, 0.12f);
      feetShape.m_radius = feetRadius;
      fixtureDefFeet.shape = &feetShape;

      b2Fixture* feet = mBody->CreateFixture(&fixtureDefFeet);

      FixtureNode* objectDataFeet = new FixtureNode(this);
      objectDataFeet->setType(ObjectTypePlayer);
      feet->SetUserData(static_cast<void*>(objectDataFeet));
   }

   // attach foot sensor shape
   b2PolygonShape polygonShape;
   polygonShape.SetAsBox(
      (width / 2.0f) / (PPM * 2.0f),
      (height / 4.0f) / (PPM * 2.0f),
      b2Vec2(0.0f, (height * 0.5f) / (PPM * 2.0f)), //(height-5) / (PPM * 2.0f)
      0.0f
   );

   b2FixtureDef footSensorFixtureDef;
   footSensorFixtureDef.isSensor = true;
   footSensorFixtureDef.shape = &polygonShape;

   b2Fixture* footSensorFixture = mBody->CreateFixture(&footSensorFixtureDef);

   FixtureNode* objectData = new FixtureNode(this);
   objectData->setType(ObjectTypePlayerFootSensor);
   footSensorFixture->SetUserData(static_cast<void*>(objectData));
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createBody()
{
   // create player body
   auto bodyDef = new b2BodyDef();
   bodyDef->position.Set(
      getPixelPosition().x * MPP,
      getPixelPosition().y * MPP
   );

   bodyDef->type = b2_dynamicBody;

   mBody = mWorld->CreateBody(bodyDef);
   mBody->SetFixedRotation(true);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createPlayerBody()
{
   createBody();
   createHead();
   createFeet();
}


//----------------------------------------------------------------------------------------------------------------------
b2World *Player::getWorld() const
{
   return mWorld;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setWorld(b2World *world)
{
   mWorld = world;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getMaxVelocity() const
{
  auto velocityMax = 0.0f;

  if (isInWater())
  {
     velocityMax = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWater;
  }
  else
  {
     if ((mKeysPressed & KeyPressedRun) || mControllerRunPressed)
     {
        velocityMax = PhysicsConfiguration::getInstance().mPlayerSpeedMaxRun;
     }
     else
     {
        velocityMax = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
     }
  }
  return velocityMax;
}

//----------------------------------------------------------------------------------------------------------------------
bool Player::isLookingAround() const
{
  if (mKeysPressed & KeyPressedLook)
  {
    return true;
  }

  if (isControllerUsed())
  {
    return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromController(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const
{
  auto axisValues = mJoystickInfo.getAxisValues();
  auto desiredVel = 0.0f;

  if (isLookingAround())
  {
    return 0.0f;
  }

  int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_LEFTX);

  // normalize to -1..1
  auto xl = axisValues[axisLeftX] / 32768.0f;
  auto hatValue = mJoystickInfo.getHatValues().at(0);
  auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
  auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;
  if (dpadLeftPressed)
  {
     xl = -1.0f;
  }
  else if (dpadRightPressed)
  {
     xl = 1.0f;
  }

  if (fabs(xl)> 0.3f)
  {
     xl *= acceleration;
     if (xl < 0.0f)
     {
        desiredVel = b2Max( velocity.x + xl, -velocityMax);
     }
     else
     {
        desiredVel = b2Min(velocity.x + xl, velocityMax);
     }
  }
  else
  {
     // if neither x axis nor dpad is used, trigger slowdown
     desiredVel = velocity.x * slowdown;
  }

  return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isMovingLeft() const
{
  if (isControllerUsed())
  {
     auto axisValues = mJoystickInfo.getAxisValues();
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[axisLeftX] / 32768.0f;
     auto hatValue = mJoystickInfo.getHatValues().at(0);
     auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
     auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

     if (dpadLeftPressed)
     {
        xl = -1.0f;
     }
     else if (dpadRightPressed)
     {
        xl = 1.0f;
     }

     if (fabs(xl) >  0.3f)
     {
        if (xl < 0.0f)
        {
           return true;
        }
     }
  }
  else
  {
     if (mKeysPressed & KeyPressedLeft)
     {
        return true;
     }
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isMovingRight() const
{
  if (isControllerUsed())
  {
     auto axisValues = mJoystickInfo.getAxisValues();
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[axisLeftX] / 32768.0f;
     auto hatValue = mJoystickInfo.getHatValues().at(0);
     auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
     auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

     if (dpadLeftPressed)
     {
        xl = -1.0f;
     }
     else if (dpadRightPressed)
     {
        xl = 1.0f;
     }

     if (fabs(xl)> 0.3f)
     {
        if (xl > 0.0f)
        {
           return true;
        }
     }
  }
  else
  {
     if (mKeysPressed & KeyPressedRight)
     {
        return true;
     }
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isMoving() const
{
  return isMovingLeft() || isMovingRight();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlayerOrientation()
{
   if (isControllerUsed())
   {
      auto axisValues = mJoystickInfo.getAxisValues();
      int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axisValues[axisLeftX] / 32768.0f;
      auto hatValue = mJoystickInfo.getHatValues().at(0);
      auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
      auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;
      if (dpadLeftPressed)
      {
         xl = -1.0f;
      }
      else if (dpadRightPressed)
      {
         xl = 1.0f;
      }

      if (fabs(xl)> 0.3f)
      {
         if (xl < 0.0f)
         {
            mPointsToLeft = true;
         }
         else
         {
            mPointsToLeft = false;
         }
      }
   }
   else
   {
      if (mKeysPressed & KeyPressedLeft)
      {
         mPointsToLeft = true;
      }

      if (mKeysPressed & KeyPressedRight)
      {
         mPointsToLeft = false;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromKeyboard(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const
{
  float desiredVel = 0.0f;

  if (mKeysPressed & KeyPressedLook)
  {
    return desiredVel;
  }

  if (mKeysPressed & KeyPressedLeft)
  {
     desiredVel = b2Max(velocity.x - acceleration, -velocityMax);
  }

  if (mKeysPressed & KeyPressedRight)
  {
     desiredVel = b2Min(velocity.x + acceleration, velocityMax);
  }

  // slowdown as soon as
  // a) no movement to left or right
  // b) movement is opposite to given direction
  // c) no movement at all
   auto noveMovementToLeftOrRight =
         (!(mKeysPressed & KeyPressedLeft))
      && (!(mKeysPressed & KeyPressedRight));

   auto velocityOppositeToGivenDir =
         (velocity.x < -0.01f && mKeysPressed & KeyPressedRight)
      || (velocity.x >  0.01f && mKeysPressed & KeyPressedLeft);

   auto noMovement = desiredVel == 0.0f;

  if (noveMovementToLeftOrRight || velocityOppositeToGivenDir || noMovement)
  {
     desiredVel = velocity.x * slowdown;
  }

  return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getSlowDown() const
{
  auto slowdown =
     (isInAir())
        ? PhysicsConfiguration::getInstance().mPlayerDecelerationAir
        : PhysicsConfiguration::getInstance().mPlayerDecelerationGround;

  return slowdown;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getAcceleration() const
{
  auto acceleration =
     (isInAir())
        ? PhysicsConfiguration::getInstance().mPlayerAccelerationAir
        : PhysicsConfiguration::getInstance().mPlayerAccelerationGround;

  return acceleration;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAnimationOffset()
{
   std::shared_ptr<Animation> nextCycle = nullptr;

   auto y = 0u;
   [[maybe_unused]] auto velocity = mBody->GetLinearVelocity().x;
   auto inAir = isInAir();
   auto inWater = isInWater();

   // run
   if (isMovingRight() && !inAir && !inWater)
   {
      y = 0;
      nextCycle = mRunRightAligned;
   }
   else if (isMovingLeft() && !inAir && !inWater)
   {
      y = PLAYER_TILES_HEIGHT;
      nextCycle = mRunLeftAligned;
   }

   // jump
   else if (!mPointsToLeft)
   {
      y = 2 * PLAYER_TILES_HEIGHT;
      nextCycle = mIdleLeftAligned;
   }
   else
   {
      y = 3 * PLAYER_TILES_HEIGHT;
      nextCycle = mIdleRightAligned;
   }

   // reset x if animation cycle changed
   if (y != mSpritePrev.y)
   {
      mSpriteAnim.x = 0;
   }

   if (nextCycle != mPreviousCycle)
   {
      if (nextCycle != nullptr)
      {
         nextCycle->seekToStart();
      }

      if (mPreviousCycle != nullptr)
      {
         mPreviousCycle->seekToStart();
      }
   }

   // update animation cycle
   if (mClock.getElapsedTime().asMilliseconds() >= mAnimSpeed)
   {
      mSpriteAnim.x++;

      if ((mSpriteAnim.x * PLAYER_TILES_WIDTH) >= PLAYER_TILES_WIDTH * PLAYER_ANIMATION_CYCLES)
      {
         mSpriteAnim.x = 0;
      }

      mClock.restart();
   }

   mSpriteAnim.y = y;
   mSpritePrev = mSpriteAnim;

   mPreviousCycle = nextCycle;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isControllerUsed() const
{
  return !mJoystickInfo.getAxisValues().empty();
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity() const
{
  auto velocityMax   = getMaxVelocity();
  auto acceleration  = getAcceleration();
  auto slowdown      = getSlowDown();
  auto velocity      = mBody->GetLinearVelocity();
  auto desiredVel    = getDesiredVelocity(velocityMax, velocity, slowdown, acceleration);
  return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const
{
  auto desiredVel = 0.0f;
  if (isControllerUsed())
  {
     // controller
     desiredVel = getVelocityFromController(velocityMax, velocity, slowdown, acceleration);
  }
  else
  {
     // keyboard
     desiredVel = getVelocityFromKeyboard(velocityMax, velocity, slowdown, acceleration);
  }

  return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::applyBeltVelocity(float& desiredVel)
{
  if (isOnBelt())
  {
     if (getBeltVelocity() < 0.0f)
     {
       if (isMovingRight())
       {
         desiredVel *= 0.5f;
       }
       else if (isMovingLeft())
       {
         if (desiredVel > 0.0f)
         {
           desiredVel = 0.0f;
         }
         desiredVel *= 2.0f;
         desiredVel = minimum(desiredVel, getMaxVelocity());
       }
       else
       {
         desiredVel += getBeltVelocity();
       }
     }
     else if (getBeltVelocity() > 0.0f)
     {
       if (isMovingLeft())
       {
         desiredVel *= 0.5f;
       }
       else if (isMovingRight())
       {
         if (desiredVel < 0.0f)
         {
           desiredVel = 0.0f;
         }
         desiredVel *= 2.0f;
         desiredVel = maximum(desiredVel, -getMaxVelocity());
       }
       else
       {
         desiredVel += getBeltVelocity();
       }
     }
  }
}


void Player::updateVelocity()
{
   auto desiredVel = getDesiredVelocity();
   auto currentVelocity = mBody->GetLinearVelocity();

   // physically so wrong but gameplay-wise the best choice :)
   applyBeltVelocity(desiredVel);

   // calc impulse, disregard time factor
   auto velChange = desiredVel - currentVelocity.x;
   auto impulse = mBody->GetMass() * velChange;

   mBody->ApplyLinearImpulse(
      b2Vec2(impulse, 0.0f),
      mBody->GetWorldCenter(),
      true
   );

   // TODO: port this to box2d buoyancy: http://www.iforce2d.net/b2dtut/buoyancy
   // limit velocity
   if (isInWater())
   {
      // const float32 speed = velocity.Length();
      auto linearVelocity = mBody->GetLinearVelocity();

      if (linearVelocity.y > 0.0f)
      {
         linearVelocity.Set(
            linearVelocity.x,
            std::min(linearVelocity.y, PhysicsConfiguration::getInstance().mPlayerSpeedMaxWater)
         );
      }
      else if (linearVelocity.y < 0.0f)
      {
         linearVelocity.Set(
            linearVelocity.x,
            std::max(linearVelocity.y, -PhysicsConfiguration::getInstance().mPlayerSpeedMaxWater)
         );
      }

      mBody->SetLinearVelocity(linearVelocity);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateJumpBuffer()
{
   // if jump is pressed while the ground is just a few centimeters away,
   // store the information and jump as soon as the places touches ground
   auto now = GlobalClock::getInstance()->getElapsedTime();
   auto timeDiff = (now - mLastJumpPressTime).asMilliseconds();

   if (!isInAir())
   {
      if (timeDiff < PhysicsConfiguration::getInstance().mPlayerJumpBufferMs)
      {
         jump();
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePortal()
{
   if (mPortalClock.getElapsedTime().asSeconds() > 1.0f)
   {
      auto axisValues = mJoystickInfo.getAxisValues();
      auto joystickPointsUp = false;
      if (!axisValues.empty())
      {
         auto dpadUpPressed = false;
         if (!mJoystickInfo.getHatValues().empty())
         {
            dpadUpPressed = mJoystickInfo.getHatValues().at(0) & SDL_HAT_UP;
         }

         auto y1 = axisValues[1] / 32768.0f;
         joystickPointsUp = (y1 < -0.4f) || dpadUpPressed;
      }

      if (
            mKeysPressed & KeyPressedUp
         || joystickPointsUp
      )
      {
         auto portal = Level::getCurrentLevel()->getNearbyPortal();
         if (portal != nullptr && portal->getDst() != nullptr)
         {
            mPortalClock.restart();

            auto dstPos =  portal->getDst()->getPortalPosition();
            setBodyViaPixelPosition(
               dstPos.x + PLAYER_ACTUAL_WIDTH / 2,
               dstPos.y + DIFF_PLAYER_TILE_TO_PHYSICS
            );
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateLostGroundContact()
{
   // when losing contact to the ground allow jumping for 2-3 more frames
   //
   // if player had ground contact in previous frame but now lost ground
   // contact then start counting to 200ms
   if (mHadGroundContact && isInAir() && !isJumping())
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      mGroundContactLostTime = now;
      mGroundContactJustLost = true;
   }

   // flying now, probably allow jump
   else if (isInAir())
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      auto timeDiff = (now - mGroundContactLostTime).asMilliseconds();
      mGroundContactJustLost = (timeDiff < PhysicsConfiguration::getInstance().mPlayerJumpAfterContactLostMs);

      // if (mGroundContactJustLost)
      // {
      //    std::cout << "allowed to jump for another " << timeDiff << "ms" << std::endl;
      // }
   }
   else
   {
      mGroundContactJustLost = false;
   }

   mHadGroundContact = !isInAir();
}


//----------------------------------------------------------------------------------------------------------------------
const GameControllerInfo& Player::getJoystickInfo() const
{
   return mJoystickInfo;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setJoystickInfo(const GameControllerInfo &joystickInfo)
{
   mJoystickInfo = joystickInfo;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::damage(int damage, const sf::Vector2f& force)
{
   if (damage == 0)
   {
      return;
   }

   if (mDamageClock.getElapsedTime().asMilliseconds() > 3000)
   {
      Audio::getInstance()->playSample(Audio::SampleHurt);

      // I'm not converting this to PPM to make the effect of the applied force more visible
      auto body = getBody();
      // body->ApplyForceToCenter(b2Vec2(force.x, force.y), true);
      body->ApplyLinearImpulse(b2Vec2(force.x / PPM, force.y / PPM), body->GetWorldCenter(), true);

      printf("player damage: %d\n", damage);
      mExtraTable->mHealth->mHealth -= damage;
      mDamageClock.restart();

      if (mExtraTable->mHealth->mHealth < 0)
      {
         printf("player is dead\n");

         // the function below is not called since 'damage(...)' is evaluated
         // within the box2d step function; no further box2d related adjustments
         // can be made until step() is finished
         //
         // mPlayerDiedCallback();
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isControllerButtonPressed(int buttonEnum) const
{
  auto pressed = false;

  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     auto buttonId = gji->getController()->getButtonId(static_cast<SDL_GameControllerButton>(buttonEnum));
     pressed = (mJoystickInfo.getButtonValues()[buttonId]);
  }

  return pressed;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateJump()
{
   auto jumpPressed = isJumpButtonPressed();

   if (isInWater() && jumpPressed)
   {
      mBody->ApplyForce(b2Vec2(0, -1.0f), mBody->GetWorldCenter(), true);
   }
   else if (mJumpSteps > 0 && jumpPressed)
   {
      // jump higher if a faster
      auto maxWalk = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
      auto vel = fabs(mBody->GetLinearVelocity().x) - maxWalk;
      auto factor = 1.0f;

      if (vel > 0.0f)
      {
         auto maxRun = PhysicsConfiguration::getInstance().mPlayerSpeedMaxRun;
         factor =
              1.0f
            + PhysicsConfiguration::getInstance().mPlayerJumpSpeedFactor
            * (vel / (maxRun - maxWalk));
      }

      /*
       * +---+
         |###|
         |###| <- current speed => factor
         |###|
         +###+
         |   |
         |   |
         |   |
       * +---+
      */

     // to change velocity by 5 in one time step
     auto force = factor * mBody->GetMass() * PhysicsConfiguration::getInstance().mPlayerJumpStrength / (1/60.0f) /*dt*/; //f = mv/t

     //spread this over 6 time steps
     force /= PhysicsConfiguration::getInstance().mPlayerJumpFalloff;

     // printf("force: %f\n", force);
     mBody->ApplyForceToCenter(b2Vec2(0,-force), true );
     mJumpSteps--;
   }
   else
   {
      mJumpSteps = 0;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlatformMovement(float dt)
{
   // http://www.badlogicgames.com/forum/viewtopic.php?f=15&t=4695&hilit=+plattform

   if (
         GameContactListener::getInstance()->getNumMovingPlatformContacts() > 0
      && GameContactListener::getInstance()->getNumFootContacts() > 0
   )
   {
      mBody->SetTransform(
         b2Vec2(
            mBody->GetPosition().x + dt * getPlatformVelocity(), // * 1.34f,
            mBody->GetPosition().y
         ),
         0.0f
      );

      // printf("standing on platform\n");
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isFireButtonPressed() const
{
  if (mKeysPressed & KeyPressedFire)
  {
    return true;
  }

  if (isControllerUsed())
  {
    return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_X);
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isJumpButtonPressed() const
{
  if (mKeysPressed & KeyPressedJump)
  {
    return true;
  }

  if (isControllerUsed())
  {
     return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isJumping() const
{
   return (mJumpSteps > 0);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateFire()
{
   if (isFireButtonPressed())
   {
      fire();
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isInWater() const
{
   return mInWater;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setInWater(bool inWater)
{
   mInWater = inWater;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateFootsteps()
{
   if (GameContactListener::getInstance()->getNumFootContacts() > 0 && !isInWater())
   {
      float vel = fabs(mBody->GetLinearVelocity().x);
      if (vel > 0.1f)
      {
         if (vel < 3.0f)
            vel = 3;

         if (mTime > mNextFootStepTime)
         {
            // play footstep
            Audio::getInstance()->playSample(Audio::SampleFootstep, 1);
            mNextFootStepTime = mTime + 1.0f / vel;
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
int Player::getId() const
{
   return mId;
}


//----------------------------------------------------------------------------------------------------------------------
int Player::getZ() const
{
   return mZ;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setZ(int z)
{
   mZ = z;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::update(float dt)
{
   mTime += dt;

   setHeadEnabled(!(mKeysPressed & KeyPressedDown)); // CLEAN UP!

   updateExtraManager();
   updateAtmosphere();
   updateFire();
   updateVelocity();
   updatePlayerOrientation();
   updateLostGroundContact();
   updateJump();
   updateJumpBuffer();
   updateDash();
   updateClimb();
   updatePlatformMovement(dt);
   updatePixelPosition();
   updateFootsteps();
   updatePortal();
   AnimationPool::getInstance().updateAnimations(dt);
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isClimbableEdge(b2ChainShape* shape, int i)
{
   /*
      climbable edges:

         c     n             c     p
         x-----+             x-----+
         |     |             |     |
         |     |             |     |
      p  +-----+ nn       n  +-----+ pp
         |     |             |     |
         |     |             |     |
      pp +-----+          nn +-----+

               p.y > c.y
            && n.x != c.x
            && pp.y > p.y

                n.y > c.y
            && p.x != c.x
            && nn.y > n.y


      c     n     nn      c     p     pp     pp    p     c      nn    n     c
      x-----+-----+       x-----+-----+      +-----+-----x      +-----+-----x
      |     |     |       |     |     |      |     |     |      |     |     |
      |     |     |       |     |     |      |     |     |      |     |     |
    p +-----+-----+     n +-----+-----+      +-----+-----+ n    +-----+-----+ p
            pp                  nn                 nn                pp

               p.y > c.y
            && n.x != c.x
            && pp.x == n.x

               n.y > c.y
            && n.x != c.x
            && p.x == nn.x


     example:

         pp: 45.5; 158.5
         p:  45.0; 158.5
         c:  44.5; 158.5
         n:  44.5; 159.0
         nn: 45.0; 159.0

         c	   p     pp
         44.5  45.0  45.5
         158.5 158.5 158.5
         +-----+-----+-----+
         |     |     |
         |     |     |
         +-----+-----+-----+
         n     nn
         44.5  45.0
         159.0 159.0

   */

   // the last vertex of a shape is duplicated for some weird reson.
   // probably needs some more investigation. for now it's just not taken into regard
   shape->m_count--;

   auto index = [shape](int i) -> int {
      if (i >= shape->m_count)
      {
         return i - shape->m_count;
      }
      if (i < 0)
      {
         return shape->m_count + i;
      }
      return i;
   };

   auto pp = shape->m_vertices[index(i - 2)];
   auto p = shape->m_vertices[index(i - 1)];
   auto c = shape->m_vertices[i];
   auto n = shape->m_vertices[index(i + 1)];
   auto nn = shape->m_vertices[index(i + 2)];

   shape->m_count++;

   auto climbable =
         (p.y > c.y && n.x != c.x && pp.y > p.y)
      || (n.y > c.y && p.x != c.x && nn.y > n.y)
      || (p.y > c.y && n.x != c.x && pp.x == n.x)
      || (n.y > c.y && p.x != c.x && p.x == nn.x);

//   if (!climbable)
//   {
//      if (shape->m_count < 100)
//      {
//         for (int i = 0; i < shape->m_count; i++)
//         {
//            printf("shape v(%d): %f, %f\n", i, shape->m_vertices[i].x, shape->m_vertices[i].y);
//         }
//      }
//      printf("boo\n");
//   }

   return climbable;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::removeClimbJoint()
{
   if (mClimbJoint)
   {
      mWorld->DestroyJoint(mClimbJoint);
      mClimbJoint = nullptr;
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::edgeMatchesMovement(const b2Vec2& edgeDir)
{
   bool rightPressed = mKeysPressed & KeyPressedRight;
   bool leftPressed = mKeysPressed & KeyPressedLeft;

   auto matchesMovement = false;
   auto edgeType = Edge::None;

   if (edgeDir.x < -0.01f)
   {
      edgeType = Edge::Right;
      matchesMovement = rightPressed;
   }
   else if (edgeDir.x > 0.01f)
   {
      edgeType = Edge::Left;
      matchesMovement = leftPressed;
   }

   return matchesMovement;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateClimb()
{
   if (!(mExtraTable->mSkills->mSkills & ExtraSkill::SkillClimb))
   {
      return;
   }

   // http://www.iforce2d.net/b2dtut/world-querying

//   // player must indicate the direction he wants to go to
//   bool rightPressed = mKeysPressed & KeyPressedRight;
//   bool leftPressed = mKeysPressed & KeyPressedLeft;
//   if (!(leftPressed || rightPressed))
//   {
//      removeJoint();
//      return;
//   }

   // if player is standing somewhere, remove joint
   if (!isInAir())
   {
      removeClimbJoint();
      return;
   }

   // remove that joint if player is moving 'up'
   if (mBody->GetLinearVelocity().y < -0.51f)
   {
      removeClimbJoint();
      return;
   }

   if (mClimbJoint != nullptr)
   {
      return;
   }


   // hold
   if (mClimbJoint != nullptr)
   {
      // remove that joint if it points down from the player perspective
      auto jointDir = (mClimbJoint->GetAnchorA() - mClimbJoint->GetAnchorB());
      if (jointDir.y < -0.1f)
      {
         removeClimbJoint();
      }
      // printf("joint dir: %f \n", jointDir.y);

      return;
   }

   // TODO: generate accurate aabb to simulate player 'hands'
   //   float smallerX = 0.0f;
   //   float smallerY = 0.0f;
   //   float largerX = 0.0f;
   //   float largerY = 0.0f;
   //
   //   b2Vec2 lower(smallerX, smallerY);
   //   b2Vec2 upper(largerX, largerY);
   //
   //   b2AABB aabb;
   //   aabb.lowerBound = lower;
   //   aabb.upperBound = upper;

   // query nearby region
   PlayerAABBQueryCallback queryCallback;


   // player aabb is: 19.504843, 158.254532 to 19.824877 158.966980
   // x: 0.320034 * 0.5 -> 0.1600171
   // y: 0.712448 * 0.5 -> 0.356224
   b2AABB aabb;
   float w = 0.1600171f;
   float h = 0.356224f;
   b2Vec2 center = mBody->GetWorldCenter();
   aabb.lowerBound = b2Vec2(center.x - w, center.y - h);
   aabb.upperBound = b2Vec2(center.x + w, center.y + h);

//   b2AABB aabb;
//   aabb.lowerBound = b2Vec2(FLT_MAX,FLT_MAX);
//   aabb.upperBound = b2Vec2(-FLT_MAX,-FLT_MAX);
//   auto fixture = mBody->GetFixtureList();
//   while (fixture != NULL)
//   {
//       aabb.Combine(aabb, fixture->GetAABB(0));
//       fixture = fixture->GetNext();
//   }

   // printf("player aabb is: %f, %f to %f %f\n", aabb.lowerBound.x, aabb.lowerBound.y, aabb.upperBound.x, aabb.upperBound.y);

   mWorld->QueryAABB(&queryCallback, aabb);

   // std::remove_if(queryCallback.foundBodies.begin(), queryCallback.foundBodies.end(), [this](b2Body* body){return body == mBody;});
   queryCallback.mBodies.erase(mBody);

   // printf("bodies in range:\n");
   for (auto body : queryCallback.mBodies)
   {
      if (body->GetType() == b2_staticBody)
      {
         // printf("- static body: %p\n", body);

         auto found = false;
         auto distMax = 0.16f;
         auto fixture = body->GetFixtureList();
         while (fixture)
         {
            auto shape = dynamic_cast<b2ChainShape*>(fixture->GetShape());

            if (shape)
            {
               b2Vec2 closest;

               auto edgeLengthMinimum = 1000.0f;
               for (auto index = 0; index < shape->m_count; index++)
               {
                  auto curr = shape->m_vertices[index];
                  auto edgeLengthSquared = (aabb.GetCenter() - curr).LengthSquared();

                  if (edgeLengthSquared >= distMax)
                  {
                     continue;
                  }

                  // does player point to edge direction?
                  if (!edgeMatchesMovement(aabb.GetCenter() - curr))
                  {
                     continue;
                  }

                  // joint in spe needs to point up, since we're holding somewhere4
                  auto jointDir = (aabb.GetCenter() - curr);
                  if  (jointDir.y <= 0.0f)
                  {
                     continue;
                  }

                  // is the analyzed edge actually climbable?
                  if (!isClimbableEdge(shape, index))
                  {
                     continue;
                  }

                  // printf("joint dir: %f \n", jointDir.y);

                  // if all that is the case, we have a joint
                  if (edgeLengthSquared < edgeLengthMinimum)
                  {
                     edgeLengthMinimum = edgeLengthSquared;
                     closest = curr;
                     found = true;
                  }
               }

               // printf("    - closest vtx: %f, %f, distance: %f\n", closest.x, closest.y, distMin);

               // situation a)
               //
               //    v
               //    +------+ prev/next, same y, greater x
               //    |
               //    |
               //    +
               //  prev/next
               //
               //  same x, greater y
               //
               //
               // situation b)
               //                                        v
               //    prev/next, same y, smaller x +------+
               //                                        |
               //                                        |
               //                                        +
               //                                      prev/next
               //
               //                                      same x, greater y
               //
               //
               // prev or current needs to have greater y
               // other vertex must have different x

               if (found)
               {
                  b2DistanceJointDef jointDefinition;
                  jointDefinition.Initialize(mBody, body, aabb.GetCenter(), closest);
                  jointDefinition.collideConnected = true;
                  // jointDefinition.dampingRatio = 0.5f;
                  // jointDefinition.frequencyHz = 5.0f;
                  // jointDefinition.length = 0.01f;

                  Audio::getInstance()->playSample(Audio::SampleImpact);
                  mClimbJoint = mWorld->CreateJoint(&jointDefinition);
               }

               // no need to continue processing
               break;
            }

            fixture = fixture->GetNext();
         }
      }
   }

   // mDistanceJoint->SetLength(distanceJoint.GetLength() * 0.99f);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::jumpImpulse()
{
   float impulse = mBody->GetMass() * 6.0f;

   mBody->ApplyLinearImpulse(
      b2Vec2(0.0f, -impulse),
      mBody->GetWorldCenter(),
      true
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Player::jumpForce()
{
  mJumpSteps = PhysicsConfiguration::getInstance().mPlayerJumpSteps;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateDash(Dash dir)
{
  if (dir == Dash::None)
  {
    dir = mDashDir;
  }
  else
  {
    mDashSteps = 20;
    mDashDir = dir;
  }

  if (mDashSteps == 0 || mDashDir == Dash::None)
  {
    return;
  }

  auto left = (dir == Dash::Left);
  mPointsToLeft = (left);
  auto dashVector = mDashSteps * mBody->GetMass() * 3.0f;
  auto impulse = (left) ? -dashVector : dashVector;

  mBody->ApplyForceToCenter(
     b2Vec2(impulse, 0.0f),
     false
  );

  mDashSteps--;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateExtraManager()
{
  mExtraManager->collide(getPlayerRect());
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAtmosphere()
{
   bool wasInwater = isInWater();

   b2Vec2 pos = mBody->GetPosition();
   PhysicsTile tile = Level::getCurrentLevel()->getPhysics().getTileForPosition(pos);

   bool inWater = tile >= PhysicsTileWaterFull && tile <= PhysicsTileWaterCornerTopLeft;
   setInWater(inWater);

   mBody->SetGravityScale(inWater ? 0.5f : 1.0f);

   if (!wasInwater && isInWater())
   {
      Audio::getInstance()->playSample(Audio::SampleSplash);
      // https://freesound.org/people/Rocktopus/packs/14347/
   }

   // not sure if this is just another ugly hack
   // when we leave the water we want to take out the current swimming velocity
   if (wasInwater && !isInWater())
   {
      mBody->SetLinearVelocity(b2Vec2(0,0));
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getPlatformVelocity() const
{
   return mPlatformVelocity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPlatformVelocity(float platformVelocity)
{
   mPlatformVelocity = platformVelocity;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::getVisible() const
{
   return mVisible;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setVisible(bool visible)
{
   mVisible = visible;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setFriction(float friction)
{
   for (b2Fixture* fixture = mBody->GetFixtureList(); fixture; fixture = fixture->GetNext())
   {
      fixture->SetFriction(friction);
   }
   mBody->ResetMassData();
}


//----------------------------------------------------------------------------------------------------------------------
Player *Player::getPlayer(int id)
{
   Player* p = nullptr;
   std::vector<Player*>::iterator it =
      std::find_if(
         sPlayerList.begin(),
         sPlayerList.end(),
         [id](auto p) { return p->mId == id; }
      );

   if (it != sPlayerList.end())
   {
      p = *it;
   }

   return p;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isInAir() const
{
   return (GameContactListener::getInstance()->getNumFootContacts() == 0) && !isInWater();
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isClimbing() const
{
   return mClimbJoint != nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::jump()
{
   sf::Time elapsed = mJumpClock.getElapsedTime();

   // only allow a new jump after a a couple of milliseconds
   if (elapsed.asMilliseconds() > 100)
   {
      if (!isInAir() || mGroundContactJustLost || mClimbJoint != nullptr)
      {
         removeClimbJoint();

         mJumpClock.restart();

         // jumpImpulse();
         jumpForce();

         if (isInWater())
         {
            // play some waterish sample?
         }
         else
         {
            AnimationPool::getInstance().add(
               mPointsToLeft
                ? "player_jump_dust_left_aligned"
                : "player_jump_dust_right_aligned",
               mPixelPosition.x,
               mPixelPosition.y
            );

            Audio::getInstance()->playSample(Audio::SampleJump);
         }
      }
      else
      {
         // player pressed jump but is still in air.
         // buffer that information to trigger the jump a few millis later.
         if (isInAir())
         {
            mLastJumpPressTime = GlobalClock::getInstance()->getElapsedTime();
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::fire()
{
   b2Vec2 dir;

   dir.x =
      mPointsToLeft
         ? - 1.0f
         :   1.0f;

   dir.y = 0.0f;

   auto xOffset = dir.x * 0.5f;
   auto yOffset = -0.1f;

   b2Vec2 pos;

   float force = 0.1f;
   dir.x = dir.x * force;
   dir.y = dir.y * force;

   pos.x = xOffset + mPixelPosition.x * MPP;
   pos.y = yOffset + mPixelPosition.y * MPP;

   mWeapon->fire(
      mWorld,
      pos,
      dir
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Player::die()
{
   Audio::getInstance()->playSample(Audio::SampleDeath);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::reset()
{
   // check for checkpoints
   // so start position could vary here

   mBody->SetLinearVelocity(b2Vec2(0,0));

   removeClimbJoint();

   setBodyViaPixelPosition(
      Level::getCurrentLevel()->getStartPosition().x,
      Level::getCurrentLevel()->getStartPosition().y
   );

   mExtraTable->mHealth->reset();
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isDead() const
{
   auto touchesSomethingDeadly = GameContactListener::getInstance()->getDeadlyContacts();
   auto tooFast = fabs(mBody->GetLinearVelocity().y) > 40;
   auto outOfHealth = mExtraTable->mHealth->mHealth < 0;
   return touchesSomethingDeadly || tooFast || outOfHealth;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setStartPixelPosition(float x, float y)
{
   setPixelPosition(x, y);
}


//----------------------------------------------------------------------------------------------------------------------
int Player::getKeysPressed() const
{
   return mKeysPressed;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setKeysPressed(int keysPressed)
{
   mKeysPressed = keysPressed;
}


//----------------------------------------------------------------------------------------------------------------------
b2Vec2 Player::getBodyPosition() const
{
   return mBody->GetPosition();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePixelPosition()
{
   // sync player sprite with with box2d data
   float x = mBody->GetPosition().x * PPM;
   float y = mBody->GetPosition().y * PPM;

   setPixelPosition(x, y);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (GameControllerIntegration::getCount() > 0)
   {
      return;
   }

   if (key == sf::Keyboard::Space)
   {
      mKeysPressed |= KeyPressedJump;
      jump();
   }

   if (key == sf::Keyboard::Return)
   {
      Level::getCurrentLevel()->toggleDoor();
   }

   if (key == sf::Keyboard::LShift)
   {
      mKeysPressed |= KeyPressedLook;
   }

   if (key == sf::Keyboard::Up)
   {
      mKeysPressed |= KeyPressedUp;
   }

   if (key == sf::Keyboard::Down)
   {
      mKeysPressed |= KeyPressedDown;
   }

   if (key == sf::Keyboard::Left)
   {
      mKeysPressed |= KeyPressedLeft;
   }

   if (key == sf::Keyboard::Right)
   {
      mKeysPressed |= KeyPressedRight;
   }

   if (key == sf::Keyboard::LAlt)
   {
      mKeysPressed |= KeyPressedRun;
   }

   if (key == sf::Keyboard::LControl)
   {
      mKeysPressed |= KeyPressedFire;
   }

   if (key == sf::Keyboard::Z)
   {
      updateDash(Dash::Left);
   }

   if (key == sf::Keyboard::X)
   {
      updateDash(Dash::Right);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::keyboardKeyReleased(sf::Keyboard::Key key)
{
   if (GameControllerIntegration::getCount() > 0)
   {
      return;
   }

   if (key == sf::Keyboard::LShift)
   {
      mKeysPressed &= ~KeyPressedLook;
   }

   if (key == sf::Keyboard::Up)
   {
      mKeysPressed &= ~KeyPressedUp;
   }

   if (key == sf::Keyboard::Down)
   {
      mKeysPressed &= ~KeyPressedDown;
   }

   if (key == sf::Keyboard::Left)
   {
      mKeysPressed &= ~KeyPressedLeft;
   }

   if (key == sf::Keyboard::Right)
   {
      mKeysPressed &= ~KeyPressedRight;
   }

   if (key == sf::Keyboard::Space)
   {
      mKeysPressed &= ~KeyPressedJump;
   }

   if (key == sf::Keyboard::LAlt)
   {
      mKeysPressed &= ~KeyPressedRun;
   }

   if (key == sf::Keyboard::LControl)
   {
      mKeysPressed &= ~KeyPressedFire;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::controllerRunButtonPressed()
{
   mControllerRunPressed = true;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::controllerRunButtonReleased()
{
   mControllerRunPressed = false;
}
