#include "player.h"

#include "animationpool.h"
#include "audio.h"
#include "camerapane.h"
#include "displaymode.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "fixturenode.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "level.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "physics/physicsconfiguration.h"
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
   setName(typeid(Player).name());

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
   mSwimRightAligned        = AnimationPool::getInstance().add("player_swim_right_aligned",         0.0f, 0.0f, true, false);
   mSwimLeftAligned         = AnimationPool::getInstance().add("player_swim_left_aligned",          0.0f, 0.0f, true, false);
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
   mAnimations.push_back(mSwimRightAligned);
   mAnimations.push_back(mSwimLeftAligned);
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

   mJump.mDustAnimation = std::bind(&Player::playDustAnimation, this);
   mJump.mRemoveClimbJoint = std::bind(&PlayerClimb::removeClimbJoint, mClimb);
   mControls.addKeypressedCallback([this](sf::Keyboard::Key key){keyPressed(key);});

   if (mTexture.loadFromFile("data/sprites/player.png"))
   {
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
            mJump.jump();
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
   if (mWeaponSystem->mSelected)
   {
      mWeaponSystem->mSelected->drawBullets(target);
   }

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
   const auto feetRadius = 0.16f / static_cast<float>(sFootCount);
   const auto feetDist = 0.0f;
   const auto feetOffset = static_cast<float>(sFootCount) * (feetRadius * 2.0f + feetDist) * 0.5f - feetRadius;

   for (auto i = 0u; i < sFootCount; i++)
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

      auto foot = mBody->CreateFixture(&fixtureDefFeet);
      mFootFixtures[i] = foot;

      auto objectDataFeet = new FixtureNode(this);
      objectDataFeet->setType(ObjectTypePlayer);
      foot->SetUserData(static_cast<void*>(objectDataFeet));
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

   // wallslide sensors
   const auto wallSlideSensorWidth = 8.0f;
   const auto wallSlideSensorHeight = 0.75f;
   const auto wallSlideSensorDistance = 0.21f;

   b2PolygonShape leftArmPolygonShape;
   leftArmPolygonShape.SetAsBox(
      wallSlideSensorWidth / (PPM * 2.0f),
      wallSlideSensorHeight / (PPM * 2.0f),
      b2Vec2(-wallSlideSensorDistance, -height / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef leftArmSensorFixtureDef;
   leftArmSensorFixtureDef.isSensor = true;
   leftArmSensorFixtureDef.shape = &leftArmPolygonShape;

   auto leftArmSensorFixture = mBody->CreateFixture(&leftArmSensorFixtureDef);

   auto leftArmObjectData = new FixtureNode(this);
   leftArmObjectData->setType(ObjectTypePlayerLeftArmSensor);
   leftArmSensorFixture->SetUserData(static_cast<void*>(leftArmObjectData));

   b2PolygonShape rightArmPolygonShape;
   rightArmPolygonShape.SetAsBox(
      wallSlideSensorWidth / (PPM * 2.0f),
      wallSlideSensorHeight / (PPM * 2.0f),
      b2Vec2(wallSlideSensorDistance, -height / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef rightArmSensorFixtureDef;
   rightArmSensorFixtureDef.isSensor = true;
   rightArmSensorFixtureDef.shape = &rightArmPolygonShape;

   auto rightArmSensorFixture = mBody->CreateFixture(&rightArmSensorFixtureDef);

   auto rightArmObjectData = new FixtureNode(this);
   rightArmObjectData->setType(ObjectTypePlayerRightArmSensor);
   rightArmSensorFixture->SetUserData(static_cast<void*>(rightArmObjectData));
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
   if (isInWater())
   {
      return PhysicsConfiguration::getInstance().mPlayerSpeedMaxWater;
   }

   // running is actually not supported
   // do we need an extra for higher speeds?
   //
   // if (mKeysPressed & KeyPressedRun)
   // {
   //    return PhysicsConfiguration::getInstance().mPlayerSpeedMaxRun;
   // }

   if (isInAir())
   {
      return PhysicsConfiguration::getInstance().mPlayerSpeedMaxAir;
   }

   return PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromController(const PlayerSpeed& speed) const
{
   auto axisValues = mControls.getJoystickInfo().getAxisValues();

   if (mControls.isLookingAround())
   {
      return 0.0f;
   }

   // analogue input normalized to -1..1
   const auto axisValue = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   auto axisValueNormalized = axisValues[static_cast<size_t>(axisValue)] / 32767.0f;

   // digital input
   const auto hatValue = mControls.getJoystickInfo().getHatValues().at(0);
   const auto dpadLeftPressed  = hatValue & SDL_HAT_LEFT;
   const auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

   if (dpadLeftPressed)
   {
      axisValueNormalized = -1.0f;
   }
   else if (dpadRightPressed)
   {
      axisValueNormalized = 1.0f;
   }

   // controller is not used, so slow down
   if (fabs(axisValueNormalized) <= 0.3f)
   {
      return speed.currentVelocity.x * speed.deceleration;
   }

   axisValueNormalized *= speed.acceleration;

   // checking for the current speed here because even if the player pushes a controller axis
   // to the left side, it might still dash to the other side with quite a strong impulse.
   // that would confuse the speed capping and would accelerate to infinity. true story.
   auto desiredVel = 0.0f;
   if (speed.currentVelocity.x < 0.0f)
   {
      desiredVel = b2Max(speed.currentVelocity.x + axisValueNormalized, -speed.velocityMax);

      // std::cout
      //    << "desired: " << desiredVel << " "
      //    << "current: " << speed.currentVelocity.x << " "
      //    << "axis value: " << axisValueNormalized << " "
      //    << "max: " << -speed.velocityMax
      //    << std::endl;
   }
   else
   {
      desiredVel = b2Min(speed.currentVelocity.x + axisValueNormalized, speed.velocityMax);

      // std::cout
      //    << "desired: " << desiredVel << " "
      //    << "current: " << speed.currentVelocity.x << " "
      //    << "axis value: " << axisValueNormalized << " "
      //    << "max: " << speed.velocityMax
      //    << std::endl;
   }

   return desiredVel;
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

   if (mControls.isControllerUsed())
   {
      auto axisValues = mControls.getJoystickInfo().getAxisValues();
      int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
      auto hatValue = mControls.getJoystickInfo().getHatValues().at(0);
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
      if (mControls.hasFlag(KeyPressedLeft))
      {
         mPointsToLeft = true;
      }

      if (mControls.hasFlag(KeyPressedRight))
      {
         mPointsToLeft = false;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromKeyboard(const PlayerSpeed& speed) const
{
   if (mControls.hasFlag(KeyPressedLook))
   {
      return 0.0f;
   }

   float desiredVel = 0.0f;

   if (mControls.hasFlag(KeyPressedLeft))
   {
      desiredVel = b2Max(speed.currentVelocity.x - speed.acceleration, -speed.velocityMax);
   }

   if (mControls.hasFlag(KeyPressedRight))
   {
      desiredVel = b2Min(speed.currentVelocity.x + speed.acceleration, speed.velocityMax);
   }

   // slowdown as soon as
   // a) no movement to left or right
   // b) movement is opposite to given direction
   // c) no movement at all
   const auto noMovementToLeftOrRight =
         (!(mControls.hasFlag(KeyPressedLeft)))
      && (!(mControls.hasFlag(KeyPressedRight)));

   const auto velocityOppositeToGivenDir =
         (speed.currentVelocity.x < -0.01f && mControls.hasFlag(KeyPressedRight))
      || (speed.currentVelocity.x >  0.01f && mControls.hasFlag(KeyPressedLeft));

   const auto noMovement = (fabs(desiredVel) < 0.0001f);

   if (noMovementToLeftOrRight || velocityOppositeToGivenDir || noMovement)
   {
      desiredVel = speed.currentVelocity.x * speed.deceleration;
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
void Player::playDustAnimation()
{
   AnimationPool::getInstance().add(
      mPointsToLeft
       ? "player_jump_dust_left_aligned"
       : "player_jump_dust_right_aligned",
      mPixelPositionf.x,
      mPixelPositionf.y
   );
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isDead() const
{
   return mDead;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isCrouching() const
{
   return mCrouching;
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
   else if (mControls.isMovingRight() && !inAir && !inWater && !lookActive)
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
   else if (mControls.isMovingLeft() && !inAir && !inWater && !lookActive)
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
      if (mJump.mJumpSteps == PhysicsConfiguration::getInstance().mPlayerJumpSteps)
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
      nextCycle = isPointingRight() ? mSwimRightAligned : mSwimLeftAligned;
   }

   if (mClimb.mClimbJoint)
   {
      // need to support climb animation
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
float Player::getDesiredVelocity() const
{
   const auto acceleration = getAcceleration();
   const auto deceleration = getDeceleration();

   const auto currentVelocity = mBody->GetLinearVelocity();
   const auto velocityMax = getMaxVelocity();

   PlayerSpeed speed
   {
      currentVelocity,
      velocityMax,
      acceleration,
      deceleration
   };

   const auto desiredVel = getDesiredVelocity(speed);
   return desiredVel;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity(const PlayerSpeed& speed) const
{
  auto desiredVel = 0.0f;

  if (mControls.isControllerUsed())
  {
     // controller
     desiredVel = getVelocityFromController(speed);
  }
  else
  {
     // keyboard
     desiredVel = getVelocityFromKeyboard(speed);
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
       if (mControls.isMovingRight())
       {
         desiredVel *= 0.5f;
       }
       else if (mControls.isMovingLeft())
       {
         if (desiredVel > 0.0f)
         {
           desiredVel = 0.0f;
         }
         desiredVel *= 2.0f;
         desiredVel = std::min(desiredVel, getMaxVelocity());
       }
       else
       {
         desiredVel += getBeltVelocity();
       }
     }
     else if (getBeltVelocity() > 0.0f)
     {
       if (mControls.isMovingLeft())
       {
         desiredVel *= 0.5f;
       }
       else if (mControls.isMovingRight())
       {
         if (desiredVel < 0.0f)
         {
           desiredVel = 0.0f;
         }
         desiredVel *= 2.0f;
         desiredVel = std::max(desiredVel, -getMaxVelocity());
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

   // we need friction to walk up diagonales
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
void Player::updatePortal()
{
   if (CameraPane::getInstance().isLookActive())
   {
      return;
   }

   if (mPortalClock.getElapsedTime().asSeconds() > 1.0f)
   {
      const auto& joystickInfo = mControls.getJoystickInfo();
      const auto& axisValues = joystickInfo.getAxisValues();
      auto joystickPointsUp = false;

      if (!axisValues.empty())
      {
         auto dpadUpPressed = false;
         if (!joystickInfo.getHatValues().empty())
         {
            dpadUpPressed = joystickInfo.getHatValues().at(0) & SDL_HAT_UP;
         }

         auto y1 = axisValues[1] / 32767.0f;
         joystickPointsUp = (y1 < -0.4f) || dpadUpPressed;
      }

      if (
            mControls.hasFlag(KeyPressedUp)
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
   if (mJump.isJumping())
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
void Player::updateFire()
{
   // disabled for now
   //
   if (mControls.isFireButtonPressed())
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

   if (mControls.isControllerUsed())
   {
      const auto& joystickInfo = mControls.getJoystickInfo();
      const auto& axisValues = joystickInfo.getAxisValues();

      int axisLeftY = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTY);
      auto yl = axisValues[static_cast<size_t>(axisLeftY)] / 32767.0f;
      const auto& hatValue = joystickInfo.getHatValues().at(0);
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
      downPressed = mControls.hasFlag(KeyPressedDown);
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
      mGroundNormal.Set(0.0f, -1.0f);
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
   mJump.update(mBody, isInAir(), isInWater(), isCrouching(), mClimb.isClimbing(), mControls);
   updateDash();
   mClimb.update(mBody, mControls, isInAir());
   updatePlatformMovement(dt);
   updatePixelPosition();
   updateFootsteps();
   updatePortal();
   updatePreviousBodyState();
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
   if (!(SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillDash))
   {
      return;
   }

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

#ifndef JUMP_GRAVITY_SCALING
      // disable gravity for player while dash is active
      // but only do this if gravity scaling is not used
      mBody->SetGravityScale(0.0f);
      mDashSteps = 30; // hardcoded to keep it working
#else

#endif
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

#ifdef JUMP_GRAVITY_SCALING
   // entering water
   if (inWater && !wasInwater)
   {
      mBody->SetGravityScale(0.5f);
   }

   // leaving water
   if (!inWater && wasInwater)
   {
      mBody->SetGravityScale(1.0f);
   }
#else
   mBody->SetGravityScale(inWater ? 0.5f : 1.0f);
#endif

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
void Player::fire()
{
   if (!mWeaponSystem->mSelected)
   {
      return;
   }

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
void Player::updateDeadFixtures()
{
   for (int32_t i = 0; i < sFootCount; i++)
   {
      mFootFixtures[i]->SetSensor(mDead);
   }

   mBodyFixture->SetSensor(mDead);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::die()
{
   mDead = true;

   updateDeadFixtures();

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
   mBody->SetGravityScale(1.0);

   mClimb.removeClimbJoint();

   setBodyViaPixelPosition(
      Level::getCurrentLevel()->getStartPosition().x,
      Level::getCurrentLevel()->getStartPosition().y
   );

   SaveState::getPlayerInfo().mExtraTable.mHealth.reset();

   // resetting any player info apart form the health doesn't make sense
   // since it's loaded from disk when the player dies
   // SaveState::getPlayerInfo().mInventory.resetKeys();

   // reset bodies passed from the contact listener
   mPlatformBody = nullptr;
   mGroundBody = nullptr;

   // reset dash
   mDashSteps = 0;
   resetDash();
   mDead = false;

   // fixtures are no longer dead
   updateDeadFixtures();
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
b2Vec2 Player::getBodyPosition() const
{
   return mBody->GetPosition();
}


//----------------------------------------------------------------------------------------------------------------------
namespace
{
   bool jumpStarted = false;
   sf::Time jumpStartTime;
   float jumpStartY = 0.0f;
   float jumpEpsilon = 0.00001f;
   float jumpPrevY = 0.0f;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::traceJumpCurve()
{
   if (mControls.isJumpButtonPressed())
   {
      if (!jumpStarted)
      {
         jumpStartTime = mTime;
         jumpStartY = mBody->GetPosition().y;
         jumpStarted = true;
         std::cout << std::endl << "time; y" << std::endl;
      }

      const auto jumpNextY = -(mBody->GetPosition().y - jumpStartY);
      if (fabs(jumpNextY - jumpPrevY) > jumpEpsilon)
      {
         std::cout
            << mTime.asSeconds() - jumpStartTime.asSeconds()
            << "; "
            << jumpNextY
            << std::endl;
      }

      jumpPrevY = jumpNextY;
   }
   else
   {
      jumpStarted = false;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::keyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Space)
   {
      mJump.jump();
   }

   if (key == sf::Keyboard::Return)
   {
      Level::getCurrentLevel()->toggleMechanisms();
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
std::shared_ptr<WeaponSystem> Player::getWeaponSystem() const
{
   return mWeaponSystem;
}


//----------------------------------------------------------------------------------------------------------------------
PlayerControls& Player::getControls()
{
   return mControls;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePixelPosition()
{
   // sync player sprite with with box2d data
   float x = mBody->GetPosition().x * PPM;
   float y = mBody->GetPosition().y * PPM;

   // traceJumpCurve();

   setPixelPosition(x, y);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePreviousBodyState()
{
   mPositionPrevious = mBody->GetPosition();
   mVelocityPrevious = mBody->GetLinearVelocity();
}


