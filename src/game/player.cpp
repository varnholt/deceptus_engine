#include "player.h"

#include "animationpool.h"
#include "audio.h"
#include "camerapane.h"
#include "displaymode.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "globalclock.h"
#include "fan.h"
#include "fixturenode.h"
#include "joystick/gamecontroller.h"
#include "laser.h"
#include "level.h"
#include "physicsconfiguration.h"
#include "playerinfo.h"
#include "savestate.h"
#include "weapon.h"
#include "weaponsystem.h"

#include <SFML/Graphics.hpp>

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
namespace  {
   uint16_t categoryBits = CategoryFriendly;
   uint16_t maskBitsStanding = CategoryBoundary | CategoryEnemyCollideWith;
   uint16_t maskBitsCrouching = CategoryEnemyCollideWith;
   int16_t groupIndex = 0;
}


//----------------------------------------------------------------------------------------------------------------------
Player* Player::sCurrent = nullptr;


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
  : GameNode(parent)
{
   sCurrent = this;

   mWeaponSystem = std::make_shared<WeaponSystem>();
   mExtraManager = std::make_shared<ExtraManager>();
}


//----------------------------------------------------------------------------------------------------------------------
Player* Player::getCurrent()
{
   return sCurrent;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::initialize()
{
   mSpriteAnim.x = PLAYER_TILES_WIDTH;
   mSpriteAnim.y = 0;

   mPortalClock.restart();
   mDamageClock.restart();

   mWeaponSystem->initialize();

   // none of the player animations are managed by the animation pool, they're just paused when finished
   mIdleRightAligned        = AnimationPool::getInstance().add("player_idle_right_aligned",         0.0f, 0.0f, true, false);
   mIdleLeftAligned         = AnimationPool::getInstance().add("player_idle_left_aligned",          0.0f, 0.0f, true, false);
   mRunRightAligned         = AnimationPool::getInstance().add("player_run_right_aligned",          0.0f, 0.0f, true, false);
   mRunLeftAligned          = AnimationPool::getInstance().add("player_run_left_aligned",           0.0f, 0.0f, true, false);
   mDashRightAligned        = AnimationPool::getInstance().add("player_dash_right_aligned",         0.0f, 0.0f, true, false);
   mDashLeftAligned         = AnimationPool::getInstance().add("player_dash_left_aligned",          0.0f, 0.0f, true, false);
   mCrouchRightAligned      = AnimationPool::getInstance().add("player_crouch_right_aligned",       0.0f, 0.0f, true, false);
   mCrouchLeftAligned       = AnimationPool::getInstance().add("player_crouch_left_aligned",        0.0f, 0.0f, true, false);

   mJumpInitRightAligned    = AnimationPool::getInstance().add("player_jump_init_right_aligned",    0.0f, 0.0f, true, false);
   mJumpUpRightAligned      = AnimationPool::getInstance().add("player_jump_up_right_aligned",      0.0f, 0.0f, true, false);
   mJumpMidairRightAligned  = AnimationPool::getInstance().add("player_jump_midair_right_aligned",  0.0f, 0.0f, true, false);
   mJumpDownRightAligned    = AnimationPool::getInstance().add("player_jump_down_right_aligned",    0.0f, 0.0f, true, false);
   mJumpLandingRightAligned = AnimationPool::getInstance().add("player_jump_landing_right_aligned", 0.0f, 0.0f, true, false);

   mJumpInitLeftAligned     = AnimationPool::getInstance().add("player_jump_init_left_aligned",     0.0f, 0.0f, true, false);
   mJumpUpLeftAligned       = AnimationPool::getInstance().add("player_jump_up_left_aligned",       0.0f, 0.0f, true, false);
   mJumpMidairLeftAligned   = AnimationPool::getInstance().add("player_jump_midair_left_aligned",   0.0f, 0.0f, true, false);
   mJumpDownLeftAligned     = AnimationPool::getInstance().add("player_jump_down_left_aligned",     0.0f, 0.0f, true, false);
   mJumpLandingLeftAligned  = AnimationPool::getInstance().add("player_jump_landing_left_aligned",  0.0f, 0.0f, true, false);

   mAnimations.push_back(mIdleRightAligned);
   mAnimations.push_back(mIdleLeftAligned);
   mAnimations.push_back(mRunRightAligned);
   mAnimations.push_back(mRunLeftAligned);
   mAnimations.push_back(mDashRightAligned);
   mAnimations.push_back(mDashLeftAligned);
   mAnimations.push_back(mCrouchRightAligned);
   mAnimations.push_back(mCrouchLeftAligned);

   mAnimations.push_back(mJumpInitRightAligned);
   mAnimations.push_back(mJumpUpRightAligned);
   mAnimations.push_back(mJumpDownRightAligned);
   mAnimations.push_back(mJumpLandingRightAligned);
   mAnimations.push_back(mJumpMidairRightAligned);

   mAnimations.push_back(mJumpInitLeftAligned);
   mAnimations.push_back(mJumpUpLeftAligned);
   mAnimations.push_back(mJumpDownLeftAligned);
   mAnimations.push_back(mJumpLandingLeftAligned);
   mAnimations.push_back(mJumpMidairLeftAligned);

   for (auto& i : mAnimations)
   {
      i->mLooped = true;
   }

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
void Player::initializeLevel()
{
   createPlayerBody();

   setBodyViaPixelPosition(
      Level::getCurrentLevel()->getStartPosition().x,
      Level::getCurrentLevel()->getStartPosition().y
   );
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
            if (GameState::getInstance().getMode() != ExecutionMode::Running)
            {
               return;
            }
            jump();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_X,
         [](){
            if (GameState::getInstance().getMode() != ExecutionMode::Running)
            {
               return;
            }
            Level::getCurrentLevel()->toggleMechanisms();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
         [this](){
            if (GameState::getInstance().getMode() != ExecutionMode::Running)
            {
               return;
            }
            updateDash(Dash::Left);
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
         [this](){
            if (GameState::getInstance().getMode() != ExecutionMode::Running)
            {
               return;
            }
            updateDash(Dash::Right);
         }
      );
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
   mWeaponSystem->mSelected->drawBullets(target);

   if (!mVisible)
   {
      return;
   }

   // dead players shouldn't flash
   if (!isDead())
   {
      // damaged player flashes
      auto time = GlobalClock::getInstance()->getElapsedTimeInMs();
      auto damageTime = mDamageClock.getElapsedTime().asMilliseconds();
      if (mDamageInitialized && time > 3000 && damageTime < 3000)
      {
         if ((damageTime / 100) % 2 == 0)
         {
            return;
         }
      }
   }

   if (mCurrentCycle)
   {
      // that y offset is to compensate the wonky box2d origin
      const auto pos = mPixelPositionf + sf::Vector2f(0, 8);

      mCurrentCycle->setPosition(pos);

      // draw dash with motion blur
      for (auto i = 0u; i < mLastAnimations.size(); i++)
      {
         auto& anim = mLastAnimations[i];
         anim.mAnimation->setPosition(anim.mPosition);
         anim.mAnimation->setAlpha(static_cast<uint8_t>(255/(2*(mLastAnimations.size()-i))));
         anim.mAnimation->draw(target);
      }

      if (isDashActive())
      {
         mLastAnimations.push_back({pos, mCurrentCycle});
      }

      mCurrentCycle->draw(target);
   }

   AnimationPool::getInstance().drawAnimations(
      target,
      {"player_jump_dust_left_aligned", "player_jump_dust_right_aligned"}
   );
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Vector2f& Player::getPixelPositionf() const
{
   return mPixelPositionf;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Vector2i& Player::getPixelPositioni() const
{
   return mPixelPositioni;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPixelPosition(float x, float y)
{
   mPixelPositionf.x = x;
   mPixelPositionf.y = y;

   mPixelPositioni.x = static_cast<int32_t>(x);
   mPixelPositioni.y = static_cast<int32_t>(y);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlayerPixelRect()
{
   sf::IntRect rect;

   const auto dh = PLAYER_TILES_HEIGHT - PLAYER_ACTUAL_HEIGHT;

   rect.left = static_cast<int>(mPixelPositionf.x) - PLAYER_ACTUAL_WIDTH / 2;
   rect.top = static_cast<int>(mPixelPositionf.y) - dh - (dh / 2);

   rect.width = PLAYER_ACTUAL_WIDTH;
   rect.height = PLAYER_ACTUAL_HEIGHT;

   mPlayerPixelRect = rect;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::IntRect& Player::getPlayerPixelRect() const
{
   return mPlayerPixelRect;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setCrouching(bool enabled)
{
   mCrouching = enabled;
   b2Filter filter = mBodyFixture->GetFilterData();
   filter.maskBits = enabled ? maskBitsCrouching : maskBitsStanding;
   mBodyFixture->SetFilterData(filter);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createFeet()
{
   // feet
   //  (   )  (   )  (   )  (   )
   //      ____      _____
   //      ^  ^      ^   ^
   //      dist      radius * 2
   //  __________________________
   //  ^                        ^
   //  count * (dist + radius)

   const auto width  = PLAYER_ACTUAL_WIDTH;
   const auto height = PLAYER_ACTUAL_HEIGHT;
   const auto feetCount = 4u;
   const auto feetRadius = 0.16f / static_cast<float>(feetCount);
   const auto feetDist = 0.0f;
   const auto feetOffset = static_cast<float>(feetCount) * (feetRadius * 2.0f + feetDist) * 0.5f - feetRadius;

   for (auto i = 0u; i < feetCount; i++)
   {
      b2FixtureDef fixtureDefFeet;
      fixtureDefFeet.density = 1.f;
      fixtureDefFeet.friction = PhysicsConfiguration::getInstance().mPlayerFriction;
      fixtureDefFeet.restitution = 0.0f;
      fixtureDefFeet.filter.categoryBits = categoryBits;
      fixtureDefFeet.filter.maskBits = maskBitsStanding;
      fixtureDefFeet.filter.groupIndex = groupIndex;

      b2CircleShape feetShape;
      feetShape.m_p.Set(i * (feetRadius * 2.0f + feetDist) - feetOffset, 0.12f);
      feetShape.m_radius = feetRadius;
      fixtureDefFeet.shape = &feetShape;

      auto feet = mBody->CreateFixture(&fixtureDefFeet);

      auto objectDataFeet = new FixtureNode(this);
      objectDataFeet->setType(ObjectTypePlayer);
      feet->SetUserData(static_cast<void*>(objectDataFeet));
   }

   // attach foot sensor shape
   b2PolygonShape footPolygonShape;
   footPolygonShape.SetAsBox(
      (width / 2.0f) / (PPM * 2.0f),
      (height / 4.0f) / (PPM * 2.0f),
      b2Vec2(0.0f, (height * 0.5f) / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef footSensorFixtureDef;
   footSensorFixtureDef.isSensor = true;
   footSensorFixtureDef.shape = &footPolygonShape;

   auto footSensorFixture = mBody->CreateFixture(&footSensorFixtureDef);

   auto footObjectData = new FixtureNode(this);
   footObjectData->setType(ObjectTypePlayerFootSensor);
   footSensorFixture->SetUserData(static_cast<void*>(footObjectData));

   // attach head sensor shape
   b2PolygonShape headPolygonShape;
   headPolygonShape.SetAsBox(
      (width / 2.0f) / (PPM * 2.0f),
      (height / 4.0f) / (PPM * 2.0f),
      b2Vec2(0.0f, -height / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef headSensorFixtureDef;
   headSensorFixtureDef.isSensor = true;
   headSensorFixtureDef.shape = &headPolygonShape;

   auto headSensorFixture = mBody->CreateFixture(&headSensorFixtureDef);

   auto headObjectData = new FixtureNode(this);
   headObjectData->setType(ObjectTypePlayerHeadSensor);
   headSensorFixture->SetUserData(static_cast<void*>(headObjectData));
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createBody()
{
   // create player body
   auto bodyDef = new b2BodyDef();
   bodyDef->position.Set(
      getPixelPositionf().x * MPP,
      getPixelPositionf().y * MPP
   );

   bodyDef->type = b2_dynamicBody;

   mBody = mWorld->CreateBody(bodyDef);
   mBody->SetFixedRotation(true);

   // add body shape
   b2FixtureDef fixtureBodyDef;
   fixtureBodyDef.density = 0.45f;
   fixtureBodyDef.friction = PhysicsConfiguration::getInstance().mPlayerFriction;
   fixtureBodyDef.restitution = 0.0f;

   fixtureBodyDef.filter.categoryBits = categoryBits;
   fixtureBodyDef.filter.maskBits = maskBitsStanding;
   fixtureBodyDef.filter.groupIndex = groupIndex;

   b2PolygonShape bodyShape;
   bodyShape.SetAsBox(0.16f, 0.3f, {0.0f, -0.2f}, 0.0f);
   fixtureBodyDef.shape = &bodyShape;

   mBodyFixture = mBody->CreateFixture(&fixtureBodyDef);

   FixtureNode* objectDataHead = new FixtureNode(this);
   objectDataHead->setType(ObjectTypePlayer);
   objectDataHead->setFlag("head", true);
   mBodyFixture->SetUserData(static_cast<void*>(objectDataHead));

   // mBody->Dump();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::createPlayerBody()
{
   createBody();
   createFeet();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setWorld(const std::shared_ptr<b2World>& world)
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
        if (isInAir())
        {
           velocityMax = PhysicsConfiguration::getInstance().mPlayerSpeedMaxAir;
        }
        else
        {
           velocityMax = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
        }
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

   // normalize to -1..1
   const auto axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   auto axisLeftXNormalized = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;

   const auto hatValue = mJoystickInfo.getHatValues().at(0);

   const auto dpadLeftPressed  = hatValue & SDL_HAT_LEFT;
   const auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

   if (dpadLeftPressed)
   {
      axisLeftXNormalized = -1.0f;
   }
   else if (dpadRightPressed)
   {
      axisLeftXNormalized = 1.0f;
   }

   if (fabs(axisLeftXNormalized) > 0.3f && !isDashActive())
   {
      axisLeftXNormalized *= acceleration;

      if (axisLeftXNormalized < 0.0f)
      {
         desiredVel = b2Max(velocity.x + axisLeftXNormalized, -velocityMax);
      }
      else
      {
         desiredVel = b2Min(velocity.x + axisLeftXNormalized, velocityMax);
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
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
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
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
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
bool Player::isPointingRight() const
{
   return !mPointsToLeft;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isPointingLeft() const
{
   return mPointsToLeft;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlayerOrientation()
{
   if (isDead())
   {
      return;
   }

   if (isControllerUsed())
   {
      auto axisValues = mJoystickInfo.getAxisValues();
      int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
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
   const auto noMovementToLeftOrRight =
         (!(mKeysPressed & KeyPressedLeft))
      && (!(mKeysPressed & KeyPressedRight));

   const auto velocityOppositeToGivenDir =
         (velocity.x < -0.01f && mKeysPressed & KeyPressedRight)
      || (velocity.x >  0.01f && mKeysPressed & KeyPressedLeft);

   const auto noMovement = (fabs(desiredVel) < 0.0001f);

   if (noMovementToLeftOrRight || velocityOppositeToGivenDir || noMovement)
   {
      desiredVel = velocity.x * slowdown;
   }

   return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDeceleration() const
{
   auto deceleration =
      (isInAir())
         ? PhysicsConfiguration::getInstance().mPlayerDecelerationAir
         : PhysicsConfiguration::getInstance().mPlayerDecelerationGround;

  return deceleration;
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
bool Player::isDead() const
{
   return mDead;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAnimation(const sf::Time& dt)
{
   if (isDead())
   {
      return;
   }

   std::shared_ptr<Animation> nextCycle = nullptr;

   auto velocity = mBody->GetLinearVelocity();
   const auto inAir = isInAir();
   const auto inWater = isInWater();

   const auto lookActive = CameraPane::getInstance().isLookActive();

   auto requiresUpdate = true;

   // dash
   if (isDashActive())
   {
      if (mDashDir == Dash::Left)
      {
         nextCycle = mDashLeftAligned;
      }
      else
      {
         nextCycle = mDashRightAligned;
      }
   }

   // run / crouch
   else if (isMovingRight() && !inAir && !inWater && !lookActive)
   {
      if (mCrouching)
      {
         nextCycle = mCrouchRightAligned;
      }
      else
      {
         nextCycle = mRunRightAligned;
      }
   }
   else if (isMovingLeft() && !inAir && !inWater && !lookActive)
   {
      if (mCrouching)
      {
         nextCycle = mCrouchLeftAligned;
      }
      else
      {
         nextCycle = mRunLeftAligned;
      }
   }

   // idle or idle crouch
   else if (mPointsToLeft)
   {
      if (mCrouching)
      {
         nextCycle = mCrouchLeftAligned;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleLeftAligned;
      }
   }
   else
   {
      if (mCrouching)
      {
         nextCycle = mCrouchRightAligned;
         requiresUpdate = false;
      }
      else
      {
         nextCycle = mIdleRightAligned;
      }
   }

   // jump init
   if (!isDashActive())
   {
      if (mJumpSteps == PhysicsConfiguration::getInstance().mPlayerJumpSteps)
      {
         // jump ignition
         mJumpAnimationReference = 0;
         nextCycle = isPointingRight() ? mJumpInitRightAligned : mJumpInitLeftAligned;
      }
      else if (inAir && !inWater)
      {
         // jump movement goes up
         if (velocity.y < -1.0f)
         {
            nextCycle = isPointingRight() ? mJumpUpRightAligned : mJumpUpLeftAligned;
            mJumpAnimationReference = 1;
         }
         // jump movement goes down
         else if (velocity.y > 1.0f)
         {
            nextCycle = isPointingRight() ? mJumpDownRightAligned : mJumpDownLeftAligned;
            mJumpAnimationReference = 2;
         }
         else
         {
            // jump midair
            if (mJumpAnimationReference == 1)
            {
               nextCycle = isPointingRight() ? mJumpMidairRightAligned : mJumpMidairLeftAligned;
            }
         }
      }
      // hard landing
      else if (mJumpAnimationReference == 2 && mHardLanding)
      {
         nextCycle = isPointingRight() ? mJumpLandingRightAligned : mJumpLandingLeftAligned;

         if (nextCycle->mCurrentFrame == static_cast<int32_t>(nextCycle->mFrames.size()) - 1)
         {
             mJumpAnimationReference = 3;
             mHardLanding = false;
             nextCycle->seekToStart();
         }
      }
   }

   // swimming - no animation provided yet.
   if (isInWater())
   {
      nextCycle = isPointingRight() ? mIdleRightAligned : mIdleLeftAligned;
   }

   // reset x if animation cycle changed
   if (nextCycle != mCurrentCycle)
   {
      nextCycle->seekToStart();
   }

   mCurrentCycle = nextCycle;

   if (requiresUpdate)
   {
      mCurrentCycle->update(dt);
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isControllerUsed() const
{
  return !mJoystickInfo.getAxisValues().empty();
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity() const
{
  const auto acceleration = getAcceleration();
  const auto deceleration = getDeceleration();

  const auto velocity = mBody->GetLinearVelocity();
  const auto velocityMax = getMaxVelocity();

  const auto desiredVel = getDesiredVelocity(velocityMax, velocity, deceleration, acceleration);
  return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity(float velocityMax, const b2Vec2& velocity, float deceleration, float acceleration) const
{
  auto desiredVel = 0.0f;

  if (isControllerUsed())
  {
     // controller
     desiredVel = getVelocityFromController(velocityMax, velocity, deceleration, acceleration);
  }
  else
  {
     // keyboard
     desiredVel = getVelocityFromKeyboard(velocityMax, velocity, deceleration, acceleration);
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


//----------------------------------------------------------------------------------------------------------------------
void Player::updateVelocity()
{
   if (isDead())
   {
      mBody->SetLinearVelocity(b2Vec2{0.0, 0.0});
      return;
   }

   // if we just landed hard on the ground, we need a break :)
   if (mHardLanding)
   {
      if (mHardLandingCycles > 1)
      {
         // if player does a hard landing on a moving platform, we don't want to reset the linear velocity.
         // maybe come up with a nice concept for this one day.
         if (isOnPlatform())
         {
            mHardLanding = false;
         }

         if (!isOnGround())
         {
            mHardLanding = false;
         }

         // std::cout << "hard landing: " << mHardLanding << " on ground: " << isOnGround() << " on platform: "<< isOnPlatform() << std::endl;
      }

      // std::cout << "reset" << std::endl;

      mBody->SetLinearVelocity({0.0, 0.0});
      return;
   }

   {
      if (isOnGround() && fabs(mGroundNormal.x) > 0.05f)
      {
         setFriction(2.0f);
      }
      else
      {
         setFriction(0.0f);
      }
   }

   auto desiredVel = getDesiredVelocity();
   auto currentVelocity = mBody->GetLinearVelocity();

   // physically so wrong but gameplay-wise the best choice :)
   applyBeltVelocity(desiredVel);

   // calc impulse, disregard time factor
   auto velocityChangeX = desiredVel - currentVelocity.x;
   auto impulseX = mBody->GetMass() * velocityChangeX;

   mBody->ApplyLinearImpulse(
      b2Vec2(impulseX, 0.0f),
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

   // cap speed
   static const auto maxSpeed = 10.0f;
   b2Vec2 vel = mBody->GetLinearVelocity();
   const auto speed = vel.Normalize();
   if (speed > maxSpeed)
   {
      // std::cout << "cap speed" << std::endl;
      mBody->SetLinearVelocity(maxSpeed * vel);
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
   if (CameraPane::getInstance().isLookActive())
   {
      return;
   }

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

         auto y1 = axisValues[1] / 32767.0f;
         joystickPointsUp = (y1 < -0.4f) || dpadUpPressed;
      }

      if (
            mKeysPressed & KeyPressedUp
         || joystickPointsUp
      )
      {
         auto portal = Level::getCurrentLevel()->getNearbyPortal();
         if (portal != nullptr && portal->getDestination() != nullptr)
         {
            mPortalClock.restart();

            auto dstPos =  portal->getDestination()->getPortalPosition();
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
void Player::impulse(float intensity)
{
   const auto dx = mVelocityPrevious.x - mBody->GetLinearVelocity().x;
   const auto dy = mVelocityPrevious.y - mBody->GetLinearVelocity().y;
   const auto horizontal = (fabs(dx) > fabs(dy));

   // std::cout
   //    << "intensity: " << intensity
   //    << " dx: " << dx
   //    << " dy: " << dy
   //    << " dir: " << (horizontal ? "x" : "y")
   //   << std::endl;

   if (horizontal)
   {
      if (intensity > 0.4f)
      {
         Level::getCurrentLevel()->getBoomEffect().boom(0.2f, 0.0f);
      }
   }

   if (intensity > 1.0f)
   {
      if (Level::getCurrentLevel()->getNearbyBouncer())
      {
         return;
      }

      Level::getCurrentLevel()->getBoomEffect().boom(0.0f, 1.0f);

      mHardLanding = true;
      mHardLandingCycles = 0;

      damage(static_cast<int>((intensity - 1.0f) * 20.0f));
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::damage(int damage, const sf::Vector2f& force)
{
   if (isDead())
   {
      return;
   }

   if (damage == 0)
   {
      return;
   }

   if (SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillInvulnerable)
   {
      return;
   }

   if (mDamageClock.getElapsedTime().asMilliseconds() > 3000)
   {
      mDamageInitialized = true;

      Audio::getInstance()->playSample("hurt.wav");

      // not converting this to PPM to make the effect of the applied force more visible
      auto body = getBody();
      body->ApplyLinearImpulse(b2Vec2(force.x / PPM, force.y / PPM), body->GetWorldCenter(), true);

      SaveState::getPlayerInfo().mExtraTable.mHealth.mHealth -= damage;
      mDamageClock.restart();

      if (SaveState::getPlayerInfo().mExtraTable.mHealth.mHealth < 0)
      {
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
     pressed = (mJoystickInfo.getButtonValues()[static_cast<size_t>(buttonId)]);
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
   else if (
         (mJumpSteps > 0 && jumpPressed)
      || mJumpClock.getElapsedTime().asMilliseconds() < PhysicsConfiguration::getInstance().mPlayerJumpMinimalDurationMs
   )
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
     auto force = factor * mBody->GetMass() * PhysicsConfiguration::getInstance().mPlayerJumpStrength / (1.0f / 60.0f) /*dt*/; //f = mv/t

     // spread this over 6 time steps
     force /= PhysicsConfiguration::getInstance().mPlayerJumpFalloff;

     // printf("force: %f\n", force);
     mBody->ApplyForceToCenter(b2Vec2(0.0f, -force), true );

     mJumpSteps--;
   }
   else
   {
      mJumpSteps = 0;
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnPlatform() const
{
   const auto onPlatform =
      GameContactListener::getInstance()->getNumMovingPlatformContacts() > 0 && isOnGround();

   return onPlatform;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnGround() const
{
   return GameContactListener::getInstance()->getNumFootContacts() > 0;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlatformMovement(const sf::Time& dt)
{
   if (isJumping())
   {
      return;
   }

   if (isOnPlatform() && mPlatformBody)
   {
      const auto dx = dt.asSeconds() * getPlatformBody()->GetLinearVelocity().x;

      const auto x = mBody->GetPosition().x + dx * 1.65f;
      const auto y = mBody->GetPosition().y;

      mBody->SetTransform(b2Vec2(x, y), 0.0f);

      // printf("standing on platform, x: %f, y: %f, dx: %f \n", x, y, dx);
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
   // disabled for now
   //
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
      auto vel = fabs(mBody->GetLinearVelocity().x);
      if (vel > 0.1f)
      {
         if (vel < 3.0f)
            vel = 3.0f;

         if (mTime.asSeconds() > mNextFootStepTime)
         {
            // play footstep
            Audio::getInstance()->playSample("footstep.wav", 0.05f);
            mNextFootStepTime = mTime.asSeconds() + 1.0f / vel;
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
void Player::updateCrouch()
{
   auto downPressed = false;

   if (isControllerUsed())
   {
      auto axisValues = mJoystickInfo.getAxisValues();
      int axisLeftY = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTY);
      auto yl = axisValues[static_cast<size_t>(axisLeftY)] / 32767.0f;
      auto hatValue = mJoystickInfo.getHatValues().at(0);
      auto dpadDownPressed = hatValue & SDL_HAT_DOWN;

      if (dpadDownPressed)
      {
         yl = 1.0f;
      }

      if (fabs(yl) >  0.3f)
      {
         if (yl > 0.0f)
         {
            downPressed = true;
         }
      }
   }
   else
   {
      downPressed = mKeysPressed & KeyPressedDown;
   }

   // if the head touches something while crouches, keep crouching
   if (mCrouching && !downPressed && (GameContactListener::getInstance()->getNumHeadContacts() > 0))
   {
      return;
   }

   setCrouching(downPressed && !isInAir());
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateHardLanding()
{
   {
      if (mHardLanding)
      {
         mHardLandingCycles++;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateGroundAngle()
{
   if (!isOnGround())
   {
      mGroundNormal.Set(0.0f, -1.0f);
      return;
   }

   // raycast down to determine terrain slope
   b2RayCastInput input;
   input.p1 = mBody->GetPosition();
   input.p2 = mBody->GetPosition() + b2Vec2(0.0f, 1.0f);
   input.maxFraction = 1.0f;

   float closestFraction = 1.0f;
   b2Vec2 intersectionNormal(0.0f, -1.0f);

   // for (b2Body* b = mWorld->GetBodyList(); b; b = b->GetNext())

   if (!mGroundBody)
   {
      return;
   }

   for (b2Fixture* f = mGroundBody->GetFixtureList(); f; f = f->GetNext())
   {
      // terrain is made out of chains, so only process those
      if (f->GetShape()->GetType() != b2Shape::e_chain)
      {
         continue;
      }

      b2RayCastOutput output;

      for (auto childIndex = 0; childIndex < f->GetShape()->GetChildCount(); childIndex++)
      {
         if (!f->RayCast(&output, input, childIndex))
            continue;

         if (output.fraction < closestFraction)
         {
            closestFraction = output.fraction;
            intersectionNormal = output.normal;
         }
      }
   }

   mGroundNormal = intersectionNormal;

   // std::cout << intersectionNormal.x << " " << intersectionNormal.y << std::endl;
   //
   // x: -0.447
   // y: -0.894

   // x:  0.0
   // y: -1.0

   // std::cout << 1.0 / intersectionNormal.y << std::endl;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::update(const sf::Time& dt)
{
   mTime += dt;

   updateGroundAngle();
   updateHardLanding();
   updatePlayerPixelRect();
   updateCrouch();
   updateAnimation(dt);
   updatePixelCollisions();
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
   updatePreviousBodyState();
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
         (p.y > c.y && (fabs(n.x - c.x) > 0.001f) && pp.y > p.y)
      || (n.y > c.y && (fabs(p.x - c.x) > 0.001f) && nn.y > n.y)
      || (p.y > c.y && (fabs(n.x - c.x) > 0.001f) && (fabs(pp.x - n.x) < 0.0001f))
      || (n.y > c.y && (fabs(p.x - c.x) > 0.001f) && (fabs(p.x - nn.x) < 0.0001f));

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
   // auto edgeType = Edge::None;

   if (edgeDir.x < -0.01f)
   {
      // edgeType = Edge::Right;
      matchesMovement = rightPressed;
   }
   else if (edgeDir.x > 0.01f)
   {
      // edgeType = Edge::Left;
      matchesMovement = leftPressed;
   }

   return matchesMovement;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateClimb()
{
   if (!(SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillClimb))
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
//   while (fixture != nullptr)
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

                  Audio::getInstance()->playSample("impact.wav");
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
   mJumpClock.restart();

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
   mJumpClock.restart();
   mJumpSteps = PhysicsConfiguration::getInstance().mPlayerJumpSteps;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::resetDash()
{
   // clear motion blur buffer
   mLastAnimations.clear();

   // reset alphas if needed
   for (auto& a: mAnimations)
   {
      a->setAlpha(255);
   }

   // re-enabled gravity for player
   mBody->SetGravityScale(1.0f);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateDash(Dash dir)
{
   // don't allow a new dash move inside water
   if (isInWater())
   {
      if (!isDashActive())
      {
         resetDash();
         return;
      }
   }

   // dir is the initial dir passed in on button press
   // Dash::None is passed in on regular updates after the initial press

   if (dir == Dash::None)
   {
      dir = mDashDir;
   }
   else
   {
      // prevent dash spam
      if (isDashActive())
      {
         return;
      }

      mDashSteps = PhysicsConfiguration::getInstance().mPlayerDashSteps;
      mDashDir = dir;

      // disable gravity for player while dash is active
      mBody->SetGravityScale(0.0f);
   }

   if (!isDashActive() || mDashDir == Dash::None)
   {
      return;
   }

   auto left = (dir == Dash::Left);
   mPointsToLeft = (left);
   auto dashVector = mDashSteps * mBody->GetMass() * PhysicsConfiguration::getInstance().mPlayerDashFactor;
   auto impulse = (left) ? -dashVector : dashVector;

   mBody->ApplyForceToCenter(
      b2Vec2(impulse, 0.0f),
      false
   );

   mDashSteps--;

   if (!isDashActive())
   {
      resetDash();
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isDashActive() const
{
   return (mDashSteps > 0);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePixelCollisions()
{
   const auto rect = getPlayerPixelRect();
   mExtraManager->collide(rect);
   Laser::collide(rect);
   Fan::collide(rect, mBody);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAtmosphere()
{
   bool wasInwater = isInWater();

   b2Vec2 pos = mBody->GetPosition();
   AtmosphereTile tile = Level::getCurrentLevel()->getPhysics().getTileForPosition(pos);

   bool inWater = tile >= AtmosphereTileWaterFull && tile <= AtmosphereTileWaterCornerTopLeft;
   setInWater(inWater);

   mBody->SetGravityScale(inWater ? 0.5f : 1.0f);

   if (!wasInwater && isInWater())
   {
      Audio::getInstance()->playSample("splash.wav");
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
b2Body* Player::getPlatformBody() const
{
   return mPlatformBody;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPlatformBody(b2Body* body)
{
   mPlatformBody = body;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setGroundBody(b2Body* body)
{
   mGroundBody = body;
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

   for (auto contact = mBody->GetContactList(); contact; contact = contact->next)
   {
      contact->contact->ResetFriction();
   }
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
   if (mCrouching)
   {
      return;
   }

   sf::Time elapsed = mJumpClock.getElapsedTime();

   // only allow a new jump after a a couple of milliseconds
   if (elapsed.asMilliseconds() > 100)
   {
      if (!isInAir() || mGroundContactJustLost || mClimbJoint != nullptr)
      {
         removeClimbJoint();

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
               mPixelPositionf.x,
               mPixelPositionf.y
            );

            Audio::getInstance()->playSample("jump.wav");
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

   pos.x = xOffset + mPixelPositionf.x * MPP;
   pos.y = yOffset + mPixelPositionf.y * MPP;

   mWeaponSystem->mSelected->fireInIntervals(
      mWorld,
      pos,
      dir
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Player::die()
{
   mDead = true;
   Audio::getInstance()->playSample("death.wav");
}


//----------------------------------------------------------------------------------------------------------------------
void Player::reset()
{
   // check for checkpoints
   // so start position could vary here
   mHardLanding = false;
   mHardLandingCycles = 0;

   mBody->SetLinearVelocity(b2Vec2(0,0));

   removeClimbJoint();

   setBodyViaPixelPosition(
      Level::getCurrentLevel()->getStartPosition().x,
      Level::getCurrentLevel()->getStartPosition().y
   );

   SaveState::getPlayerInfo().mExtraTable.mHealth.reset();
   SaveState::getPlayerInfo().mInventory.resetKeys();

   // reset bodies passed from the contact listener
   mPlatformBody = nullptr;
   mGroundBody = nullptr;

   // reset dash
   mDashSteps = 0;
   resetDash();
   mDead = false;
}


//----------------------------------------------------------------------------------------------------------------------
DeathReason Player::checkDead() const
{
   DeathReason reason = DeathReason::None;

   const auto touchesSomethingDeadly = (GameContactListener::getInstance()->getDeadlyContacts() > 0);
   const auto tooFast = fabs(mBody->GetLinearVelocity().y) > 40;
   const auto outOfHealth = SaveState::getPlayerInfo().mExtraTable.mHealth.mHealth <= 0;

   if (touchesSomethingDeadly)
   {
      reason = DeathReason::TouchesDeadly;
      // std::cout << "dead: touches something deadly" << std::endl;
   }
   else if (tooFast)
   {
      reason = DeathReason::TooFast;
      // std::cout << "dead: too fast" << std::endl;
   }
   else if (outOfHealth)
   {
      reason = DeathReason::OutOfHealth;
      // std::cout << "dead: out of health" << std::endl;
   }

   return reason;
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
void Player::updatePreviousBodyState()
{
   mPositionPrevious = mBody->GetPosition();
   mVelocityPrevious = mBody->GetLinearVelocity();
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
      Level::getCurrentLevel()->toggleMechanisms();
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
