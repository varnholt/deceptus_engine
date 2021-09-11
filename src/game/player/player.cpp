#include "player.h"

#include "animationpool.h"
#include "audio.h"
#include "camerapane.h"
#include "displaymode.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "fadetransitioneffect.h"
#include "framework/tools/stopwatch.h"
#include "fixturenode.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "level.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "physics/physicsconfiguration.h"
#include "playerinfo.h"
#include "savestate.h"
#include "screentransition.h"
#include "texturepool.h"
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

   mJump._dust_animation_callback = std::bind(&Player::playDustAnimation, this);
   mJump._remove_climb_joint_callback = std::bind(&PlayerClimb::removeClimbJoint, mClimb);
   mControls.addKeypressedCallback([this](sf::Keyboard::Key key){keyPressed(key);});

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
void Player::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   if (mWeaponSystem->mSelected)
   {
      mWeaponSystem->mSelected->draw(color);
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

   auto current_cycle = _player_animation.getCurrentCycle();
   if (current_cycle)
   {
      // that y offset is to compensate the wonky box2d origin
      const auto pos = mPixelPositionf + sf::Vector2f(0, 8);

      current_cycle->setPosition(pos);

      // draw dash with motion blur
      for (auto i = 0u; i < mLastAnimations.size(); i++)
      {
         auto& anim = mLastAnimations[i];
         anim.mAnimation->setPosition(anim.mPosition);
         anim.mAnimation->setAlpha(static_cast<uint8_t>(255/(2*(mLastAnimations.size()-i))));
         anim.mAnimation->draw(color);
      }

      if (isDashActive())
      {
         mLastAnimations.push_back({pos, current_cycle});
      }

      current_cycle->draw(color, normal);
   }

   AnimationPool::getInstance().drawAnimations(
      color,
      normal,
      {"player_jump_dust_l", "player_jump_dust_r"}
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
void Player::setMaskBitsCrouching(bool enabled)
{
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
      objectDataFeet->setFlag("foot", true);
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

   // store body inside player jump
   mJump._body = mBody;
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
void Player::resetWorld()
{
   mWorld.reset();
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

   // sanity check to avoid moonwalking
   if (mControls.hasFlag(KeyPressedLeft) && mControls.hasFlag(KeyPressedRight))
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
       ? "player_jump_dust_l"
       : "player_jump_dust_r",
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
   return _bending_down;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAnimation(const sf::Time& dt)
{
   PlayerAnimation::PlayerAnimationData data;

   data._dead = isDead();
   data._in_air = isInAir();
   data._in_water = isInWater();
   data._linear_velocity = mBody->GetLinearVelocity();
   data._hard_landing = mHardLanding;
   data._bending_down = _bending_down;
   data._crouching = _crouching;
   data._points_left = mPointsToLeft;
   data._points_right = !mPointsToLeft;
   data._climb_joint_present = mClimb._climb_joint;
   data._jump_frame_count = mJump._jump_frame_count;
   data._dash_frame_count = mDashFrameCount;
   data._moving_left = mControls.isMovingLeft();
   data._moving_right = mControls.isMovingRight();
   data._wall_sliding = mJump._wallsliding;
   data._wall_jump_points_right = mJump._walljump_points_right;
   data._timepoint_doublejump = mJump._timepoint_doublejump;
   data._timepoint_wallslide = mJump._timepoint_wallslide;
   data._timepoint_walljump = mJump._timepoint_walljump;
   data._timepoint_bend_down_start = _timepoint_bend_down_start;
   data._timepoint_bend_down_end = _timepoint_bend_down_end;

   if (isDashActive())
   {
      data._dash_dir = mDashDir;
   }

   _player_animation.update(dt, data);

   // reset hard landing if cycle reached
   if (mHardLanding && _player_animation.getJumpAnimationReference() == 3)
   {
      mHardLanding = false;
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

   if (Portal::isLocked())
   {
      mBody->SetLinearVelocity(b2Vec2{0.0, 0.0});
      return;
   }

   if (_bending_down)
   {
      if (!(SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillCrouch))
      {
         mBody->SetLinearVelocity(b2Vec2{0.0, 0.0});
         return;
      }

      // from here the player is crouching
      _was_crouching = _crouching;
      _crouching = true;
   }
   else
   {
      _was_crouching = _crouching;
      _crouching = false;
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
            Portal::lock();
            mPortalClock.restart();

            auto screen_transition = std::make_unique<ScreenTransition>();
            auto fade_out = std::make_shared<FadeTransitionEffect>();
            auto fade_in = std::make_shared<FadeTransitionEffect>();
            fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
            fade_out->_speed = 2.0f;
            fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
            fade_in->_value = 1.0f;
            fade_in->_speed = 2.0f;
            screen_transition->_effect_1 = fade_out;
            screen_transition->_effect_2 = fade_in;
            screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
            screen_transition->startEffect1();

            screen_transition->_callbacks_effect_1_ended.push_back(
               [this, portal](){
                  auto dstPos =  portal->getDestination()->getPortalPosition();
                  setBodyViaPixelPosition(
                     dstPos.x + PLAYER_ACTUAL_WIDTH / 2,
                     dstPos.y + DIFF_PLAYER_TILE_TO_PHYSICS
                  );

                  // update the camera system to point to the player position immediately
                  CameraSystem::getCameraSystem().syncNow();
                  Portal::unlock();
               }
            );

            screen_transition->_callbacks_effect_2_ended.push_back(
               [](){
                  ScreenTransitionHandler::getInstance()._transition.reset();
               }
            );

            ScreenTransitionHandler::getInstance()._transition = std::move(screen_transition);
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::impulse(float intensity)
{
   // just store the information we get from the post solve call for now.
   // other evaluation from BeginContact / EndContact might make the impulse irrelevant
   mImpulse = intensity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateImpulse()
{
   if (mImpulse < 0.0000001f)
   {
      return;
   }

   auto impulse = mImpulse;
   mImpulse = 0.0f;

   if (GameContactListener::getInstance()->isSmashed())
   {
      return;
   }

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
      if (impulse > 0.4f)
      {
         Level::getCurrentLevel()->getBoomEffect().boom(0.2f, 0.0f);
      }
   }

   if (impulse > 1.0f)
   {
      if (Level::getCurrentLevel()->getNearbyBouncer())
      {
         return;
      }

      Level::getCurrentLevel()->getBoomEffect().boom(0.0f, 1.0f);

      mHardLanding = true;
      mHardLandingCycles = 0;

      damage(static_cast<int>((impulse - 1.0f) * 20.0f));
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
void Player::updateBendDown()
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
   if (_bending_down && !downPressed && (GameContactListener::getInstance()->getNumHeadContacts() > 0))
   {
      return;
   }

   if (!_bending_down && CameraPane::getInstance().isLookActive())
   {
      return;
   }

   const auto bending_down = downPressed && !isInAir();

   _was_bending_down = _bending_down;
   _bending_down = bending_down;

   if (!_was_bending_down && _bending_down)
   {
      _timepoint_bend_down_start = StopWatch::getInstance().now();
   }

   if (_was_bending_down && !_bending_down)
   {
      _timepoint_bend_down_end = StopWatch::getInstance().now();
   }

   setMaskBitsCrouching(bending_down);
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

   updateImpulse();
   updateGroundAngle();
   updateHardLanding();
   updatePlayerPixelRect();
   updateBendDown();
   updateAnimation(dt);
   updatePixelCollisions();
   updateAtmosphere();
   updateFire();
   updateVelocity();
   updatePlayerOrientation();

   PlayerJump::PlayerJumpInfo info;
   info._in_air = isInAir();
   info._in_water = isInWater();
   info._crouching = isCrouching();
   info._climbing = mClimb.isClimbing();
   mJump.update(info, mControls);

   updateDash();
   mClimb.update(mBody, mControls, isInAir());
   updatePlatformMovement(dt);
   updatePixelPosition();
   updateFootsteps();
   updatePortal();
   updatePreviousBodyState();
   updateWeapons(dt);
   mControls.update(dt); // called at last just to backup previous controls
}


//----------------------------------------------------------------------------------------------------------------------
void Player::resetDash()
{
   // clear motion blur buffer
   mLastAnimations.clear();

   _player_animation.resetAlpha();

   // re-enabled gravity for player
   if (mBody)
   {
      mBody->SetGravityScale(1.0f);
   }
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

      mDashFrameCount = PhysicsConfiguration::getInstance().mPlayerDashFrameCount;
      mDashMultiplier = PhysicsConfiguration::getInstance().mPlayerDashMultiplier;
      mDashDir = dir;

#ifndef JUMP_GRAVITY_SCALING
      // disable gravity for player while dash is active
      // but only do this if gravity scaling is not used
      mBody->SetGravityScale(0.0f);
      mDashSteps = 30; // hardcoded to keep it working
#endif
   }

   if (!isDashActive() || mDashDir == Dash::None)
   {
      return;
   }

   auto left = (dir == Dash::Left);
   mPointsToLeft = (left);

   mDashMultiplier += PhysicsConfiguration::getInstance().mPlayerDashMultiplierIncrementPerFrame;
   mDashMultiplier *=PhysicsConfiguration::getInstance().mPlayerDashMultiplierScalePerFrame;

   auto dashVector = mDashMultiplier * mBody->GetMass() * PhysicsConfiguration::getInstance().mPlayerDashVector;
   auto impulse = (left) ? -dashVector : dashVector;

   mBody->ApplyForceToCenter(b2Vec2(impulse, 0.0f), false);

   mDashFrameCount--;

   if (!isDashActive())
   {
      resetDash();
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isDashActive() const
{
   return (mDashFrameCount > 0);
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

   // the force applied really depends on the weapon
   // it might make sense to have a `virtual float forceFactor() const`
   float force = 1.5f;

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
   if (!mBodyFixture)
   {
      return;
   }

   for (int32_t i = 0; i < sFootCount; i++)
   {
      mFootFixtures[i]->SetSensor(mDead);
   }

   mBodyFixture->SetSensor(mDead);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateWeapons(const sf::Time& dt)
{
   for (auto& w : mWeaponSystem->mWeapons)
   {
      w->update(dt);
   }
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

   if (mBody)
   {
      mBody->SetLinearVelocity(b2Vec2(0,0));
      mBody->SetGravityScale(1.0);
   }

   mClimb.removeClimbJoint();

   if (Level::getCurrentLevel())
   {
      setBodyViaPixelPosition(
         Level::getCurrentLevel()->getStartPosition().x,
         Level::getCurrentLevel()->getStartPosition().y
      );
   }

   SaveState::getPlayerInfo().mExtraTable.mHealth.reset();

   // resetting any player info apart form the health doesn't make sense
   // since it's loaded from disk when the player dies
   // SaveState::getPlayerInfo().mInventory.resetKeys();

   // reset bodies passed from the contact listener
   mPlatformBody = nullptr;
   mGroundBody = nullptr;

   // reset dash
   mDashFrameCount = 0;
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
   const auto smashed = GameContactListener::getInstance()->isSmashed();

   if (touchesSomethingDeadly)
   {
      reason = DeathReason::TouchesDeadly;
   }
   else if (smashed)
   {
      reason = DeathReason::Smashed;
   }
   else if (tooFast)
   {
      reason = DeathReason::TooFast;
   }
   else if (outOfHealth)
   {
      reason = DeathReason::OutOfHealth;
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
void Player::traceJumpCurve()
{
   if (mControls.isJumpButtonPressed())
   {
      if (!mJumpTrace.jumpStarted)
      {
         mJumpTrace.jumpStartTime = mTime;
         mJumpTrace.jumpStartY = mBody->GetPosition().y;
         mJumpTrace.jumpStarted = true;
         std::cout << std::endl << "time; y" << std::endl;
      }

      const auto jumpNextY = -(mBody->GetPosition().y - mJumpTrace.jumpStartY);
      if (fabs(jumpNextY - mJumpTrace.jumpPrevY) > mJumpTrace.jumpEpsilon)
      {
         std::cout
            << mTime.asSeconds() - mJumpTrace.jumpStartTime.asSeconds()
            << "; "
            << jumpNextY
            << std::endl;
      }

      mJumpTrace.jumpPrevY = jumpNextY;
   }
   else
   {
      mJumpTrace.jumpStarted = false;
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
PlayerAnimation& Player::getPlayerAnimation()
{
   return _player_animation;
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


