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
static constexpr uint16_t category_bits = CategoryFriendly;
static constexpr uint16_t mask_bits_standing = CategoryBoundary | CategoryEnemyCollideWith;
static constexpr uint16_t mask_bits_crouching = CategoryEnemyCollideWith;
static constexpr int16_t group_index = 0;
static constexpr auto impulse_epsilon = 0.0000001f;
}


//----------------------------------------------------------------------------------------------------------------------
Player* Player::__current = nullptr;


//----------------------------------------------------------------------------------------------------------------------
b2Body* Player::getBody() const
{
    return _body;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getBeltVelocity() const
{
  return _belt_velocity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setBeltVelocity(float beltVelocity)
{
  _belt_velocity = beltVelocity;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnBelt() const
{
  return _is_on_belt;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setOnBelt(bool onBelt)
{
  _is_on_belt = onBelt;
}


//----------------------------------------------------------------------------------------------------------------------
Player::Player(GameNode* parent)
  : GameNode(parent)
{
   setName(typeid(Player).name());

   __current = this;

   _weapon_system = std::make_shared<WeaponSystem>();
   _extra_manager = std::make_shared<ExtraManager>();
}


//----------------------------------------------------------------------------------------------------------------------
Player* Player::getCurrent()
{
   return __current;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::initialize()
{
   _portal_clock.restart();
   _damage_clock.restart();

   _weapon_system->initialize();

   _jump._dust_animation_callback = std::bind(&Player::playDustAnimation, this);
   _jump._remove_climb_joint_callback = std::bind(&PlayerClimb::removeClimbJoint, _climb);
   _controls.addKeypressedCallback([this](sf::Keyboard::Key key){keyPressed(key);});

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
            _jump.jump();
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
  return _extra_manager;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setBodyViaPixelPosition(float x, float y)
{
   setPixelPosition(x, y);

   if (_body)
   {
      _body->SetTransform(
         b2Vec2(x * MPP, y * MPP),
         0.0f
      );
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   if (_weapon_system->_selected)
   {
      _weapon_system->_selected->draw(color);
   }

   if (!_visible)
   {
      return;
   }

   // dead players shouldn't flash
   if (!isDead())
   {
      // damaged player flashes
      auto time = GlobalClock::getInstance().getElapsedTimeInMs();
      auto damageTime = _damage_clock.getElapsedTime().asMilliseconds();
      if (_damage_initialized && time > 3000 && damageTime < 3000)
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
      const auto pos = _pixel_position_f + sf::Vector2f(0, 8);

      current_cycle->setPosition(pos);

      // draw dash with motion blur
      for (auto i = 0u; i < _last_animations.size(); i++)
      {
         auto& anim = _last_animations[i];
         anim.mAnimation->setPosition(anim.mPosition);
         anim.mAnimation->setAlpha(static_cast<uint8_t>(255/(2*(_last_animations.size()-i))));
         anim.mAnimation->draw(color);
      }

      if (_dash.isDashActive())
      {
         _last_animations.push_back({pos, current_cycle});
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
   return _pixel_position_f;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Vector2i& Player::getPixelPositioni() const
{
   return _pixel_position_i;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPixelPosition(float x, float y)
{
   _pixel_position_f.x = x;
   _pixel_position_f.y = y;

   _pixel_position_i.x = static_cast<int32_t>(x);
   _pixel_position_i.y = static_cast<int32_t>(y);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlayerPixelRect()
{
   sf::IntRect rect;

   const auto dh = PLAYER_TILES_HEIGHT - PLAYER_ACTUAL_HEIGHT;

   rect.left = static_cast<int>(_pixel_position_f.x) - PLAYER_ACTUAL_WIDTH / 2;
   rect.top = static_cast<int>(_pixel_position_f.y) - dh - (dh / 2);

   rect.width = PLAYER_ACTUAL_WIDTH;
   rect.height = PLAYER_ACTUAL_HEIGHT;

   _pixel_rect = rect;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::IntRect& Player::getPlayerPixelRect() const
{
   return _pixel_rect;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setMaskBitsCrouching(bool enabled)
{
   b2Filter filter = _body_fixture->GetFilterData();
   filter.maskBits = enabled ? mask_bits_crouching : mask_bits_standing;
   _body_fixture->SetFilterData(filter);
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
   const auto feetRadius = 0.16f / static_cast<float>(__foot_count);
   const auto feetDist = 0.0f;
   const auto feetOffset = static_cast<float>(__foot_count) * (feetRadius * 2.0f + feetDist) * 0.5f - feetRadius;

   for (auto i = 0u; i < __foot_count; i++)
   {
      b2FixtureDef fixtureDefFeet;
      fixtureDefFeet.density = 1.f;
      fixtureDefFeet.friction = PhysicsConfiguration::getInstance()._player_friction;
      fixtureDefFeet.restitution = 0.0f;
      fixtureDefFeet.filter.categoryBits = category_bits;
      fixtureDefFeet.filter.maskBits = mask_bits_standing;
      fixtureDefFeet.filter.groupIndex = group_index;

      b2CircleShape feetShape;
      feetShape.m_p.Set(i * (feetRadius * 2.0f + feetDist) - feetOffset, 0.12f);
      feetShape.m_radius = feetRadius;
      fixtureDefFeet.shape = &feetShape;

      auto foot = _body->CreateFixture(&fixtureDefFeet);
      _foot_fixture[i] = foot;

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

   auto footSensorFixture = _body->CreateFixture(&footSensorFixtureDef);

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

   auto headSensorFixture = _body->CreateFixture(&headSensorFixtureDef);

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

   auto leftArmSensorFixture = _body->CreateFixture(&leftArmSensorFixtureDef);

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

   auto rightArmSensorFixture = _body->CreateFixture(&rightArmSensorFixtureDef);

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

   _body = _world->CreateBody(bodyDef);
   _body->SetFixedRotation(true);

   // add body shape
   b2FixtureDef fixtureBodyDef;
   fixtureBodyDef.density = 0.45f;
   fixtureBodyDef.friction = PhysicsConfiguration::getInstance()._player_friction;
   fixtureBodyDef.restitution = 0.0f;

   fixtureBodyDef.filter.categoryBits = category_bits;
   fixtureBodyDef.filter.maskBits = mask_bits_standing;
   fixtureBodyDef.filter.groupIndex = group_index;

   b2PolygonShape bodyShape;
   bodyShape.SetAsBox(0.16f, 0.3f, {0.0f, -0.2f}, 0.0f);
   fixtureBodyDef.shape = &bodyShape;

   _body_fixture = _body->CreateFixture(&fixtureBodyDef);

   FixtureNode* objectDataHead = new FixtureNode(this);
   objectDataHead->setType(ObjectTypePlayer);
   objectDataHead->setFlag("head", true);
   _body_fixture->SetUserData(static_cast<void*>(objectDataHead));

   // mBody->Dump();

   // store body inside player jump
   _jump._body = _body;
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
   _world = world;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::resetWorld()
{
   _world.reset();
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getMaxVelocity() const
{
   if (isInWater())
   {
      return PhysicsConfiguration::getInstance()._player_speed_max_water;
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
      return PhysicsConfiguration::getInstance()._player_speed_max_air;
   }

   return PhysicsConfiguration::getInstance()._player_speed_max_walk;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromController(const PlayerSpeed& speed) const
{
   auto axisValues = _controls.getJoystickInfo().getAxisValues();

   if (_controls.isLookingAround())
   {
      return 0.0f;
   }

   // analogue input normalized to -1..1
   const auto axisValue = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   auto axisValueNormalized = axisValues[static_cast<size_t>(axisValue)] / 32767.0f;

   // digital input
   const auto hatValue = _controls.getJoystickInfo().getHatValues().at(0);
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
   return !_points_to_left;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isPointingLeft() const
{
   return _points_to_left;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlayerOrientation()
{
   if (isDead())
   {
      return;
   }

   if (_controls.isControllerUsed())
   {
      auto axisValues = _controls.getJoystickInfo().getAxisValues();
      int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
      auto hatValue = _controls.getJoystickInfo().getHatValues().at(0);
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
            _points_to_left = true;
         }
         else
         {
            _points_to_left = false;
         }
      }
   }
   else
   {
      if (_controls.hasFlag(KeyPressedLeft))
      {
         _points_to_left = true;
      }

      if (_controls.hasFlag(KeyPressedRight))
      {
         _points_to_left = false;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromKeyboard(const PlayerSpeed& speed) const
{
   if (_controls.hasFlag(KeyPressedLook))
   {
      return 0.0f;
   }

   // sanity check to avoid moonwalking
   if (_controls.hasFlag(KeyPressedLeft) && _controls.hasFlag(KeyPressedRight))
   {
      return 0.0f;
   }

   float desiredVel = 0.0f;

   if (_controls.hasFlag(KeyPressedLeft))
   {
      desiredVel = b2Max(speed.currentVelocity.x - speed.acceleration, -speed.velocityMax);
   }

   if (_controls.hasFlag(KeyPressedRight))
   {
      desiredVel = b2Min(speed.currentVelocity.x + speed.acceleration, speed.velocityMax);
   }

   // slowdown as soon as
   // a) no movement to left or right
   // b) movement is opposite to given direction
   // c) no movement at all
   const auto noMovementToLeftOrRight =
         (!(_controls.hasFlag(KeyPressedLeft)))
      && (!(_controls.hasFlag(KeyPressedRight)));

   const auto velocityOppositeToGivenDir =
         (speed.currentVelocity.x < -0.01f && _controls.hasFlag(KeyPressedRight))
      || (speed.currentVelocity.x >  0.01f && _controls.hasFlag(KeyPressedLeft));

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
         ? PhysicsConfiguration::getInstance()._player_deceleration_air
         : PhysicsConfiguration::getInstance()._player_deceleration_ground;

  return deceleration;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getAcceleration() const
{
   auto acceleration =
      (isInAir())
         ? PhysicsConfiguration::getInstance()._player_acceleration_air
         : PhysicsConfiguration::getInstance()._player_acceleration_ground;

   return acceleration;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::playDustAnimation()
{
   AnimationPool::getInstance().add(
      _points_to_left
       ? "player_jump_dust_l"
       : "player_jump_dust_r",
      _pixel_position_f.x,
      _pixel_position_f.y
   );
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isDead() const
{
   return _dead;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAnimation(const sf::Time& dt)
{
   PlayerAnimation::PlayerAnimationData data;

   data._dead = isDead();
   data._in_air = isInAir();
   data._in_water = isInWater();
   data._linear_velocity = _body->GetLinearVelocity();
   data._hard_landing = _hard_landing;
   data._bending_down = _bend._bending_down;
   data._crouching = _bend._crouching;
   data._points_left = _points_to_left;
   data._points_right = !_points_to_left;
   data._climb_joint_present = _climb._climb_joint;
   data._jump_frame_count = _jump._jump_frame_count;
   data._dash_frame_count = _dash._dash_frame_count;
   data._moving_left = _controls.isMovingLeft();
   data._moving_right = _controls.isMovingRight();
   data._wall_sliding = _jump._wallsliding;
   data._wall_jump_points_right = _jump._walljump_points_right;
   data._timepoint_doublejump = _jump._timepoint_doublejump;
   data._timepoint_wallslide = _jump._timepoint_wallslide;
   data._timepoint_walljump = _jump._timepoint_walljump;
   data._timepoint_bend_down_start = _bend._timepoint_bend_down_start;
   data._timepoint_bend_down_end = _bend._timepoint_bend_down_end;

   if (_dash.isDashActive())
   {
      data._dash_dir = _dash._dash_dir;
   }

   _player_animation.update(dt, data);

   // reset hard landing if cycle reached
   if (_hard_landing && _player_animation.getJumpAnimationReference() == 3)
   {
      _hard_landing = false;
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity() const
{
   const auto acceleration = getAcceleration();
   const auto deceleration = getDeceleration();

   const auto currentVelocity = _body->GetLinearVelocity();
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

  if (_controls.isControllerUsed())
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
       if (_controls.isMovingRight())
       {
         desiredVel *= 0.5f;
       }
       else if (_controls.isMovingLeft())
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
       if (_controls.isMovingLeft())
       {
         desiredVel *= 0.5f;
       }
       else if (_controls.isMovingRight())
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
      _body->SetLinearVelocity(b2Vec2{0.0, 0.0});
      return;
   }

   if (Portal::isLocked())
   {
      _body->SetLinearVelocity(b2Vec2{0.0, 0.0});
      return;
   }

   if (_bend._bending_down)
   {
      if (!(SaveState::getPlayerInfo().mExtraTable._skills._skills & ExtraSkill::SkillCrouch))
      {
         _body->SetLinearVelocity(b2Vec2{0.0, 0.0});
         return;
      }

      // from here the player is crouching
      _bend._was_crouching = _bend._crouching;
      _bend._crouching = true;
   }
   else
   {
      _bend._was_crouching = _bend._crouching;
      _bend._crouching = false;
   }

   // if we just landed hard on the ground, we need a break :)
   if (_hard_landing)
   {
      if (_hard_landing_cycles > 1)
      {
         // if player does a hard landing on a moving platform, we don't want to reset the linear velocity.
         // maybe come up with a nice concept for this one day.
         if (isOnPlatform())
         {
            _hard_landing = false;
         }

         if (!isOnGround())
         {
            _hard_landing = false;
         }

         // std::cout << "hard landing: " << mHardLanding << " on ground: " << isOnGround() << " on platform: "<< isOnPlatform() << std::endl;
      }

      // std::cout << "reset" << std::endl;

      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   // we need friction to walk up diagonales
   {
      if (isOnGround() && fabs(_ground_normal.x) > 0.05f)
      {
         setFriction(2.0f);
      }
      else
      {
         setFriction(0.0f);
      }
   }

   auto desiredVel = getDesiredVelocity();
   auto currentVelocity = _body->GetLinearVelocity();

   // physically so wrong but gameplay-wise the best choice :)
   applyBeltVelocity(desiredVel);

   // calc impulse, disregard time factor
   auto velocityChangeX = desiredVel - currentVelocity.x;
   auto impulseX = _body->GetMass() * velocityChangeX;

   _body->ApplyLinearImpulse(
      b2Vec2(impulseX, 0.0f),
      _body->GetWorldCenter(),
      true
   );

   // TODO: port this to box2d buoyancy: http://www.iforce2d.net/b2dtut/buoyancy
   // limit velocity
   if (isInWater())
   {
      // const float32 speed = velocity.Length();
      auto linearVelocity = _body->GetLinearVelocity();

      if (linearVelocity.y > 0.0f)
      {
         linearVelocity.Set(
            linearVelocity.x,
            std::min(linearVelocity.y, PhysicsConfiguration::getInstance()._player_speed_max_water)
         );
      }
      else if (linearVelocity.y < 0.0f)
      {
         linearVelocity.Set(
            linearVelocity.x,
            std::max(linearVelocity.y, -PhysicsConfiguration::getInstance()._player_speed_max_water)
         );
      }

      _body->SetLinearVelocity(linearVelocity);
   }

   // cap speed
   static const auto maxSpeed = 10.0f;
   b2Vec2 vel = _body->GetLinearVelocity();
   const auto speed = vel.Normalize();
   if (speed > maxSpeed)
   {
      // std::cout << "cap speed" << std::endl;
      _body->SetLinearVelocity(maxSpeed * vel);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePortal()
{
   if (CameraPane::getInstance().isLookActive())
   {
      return;
   }

   if (_portal_clock.getElapsedTime().asSeconds() > 1.0f)
   {
      const auto& joystickInfo = _controls.getJoystickInfo();
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
            _controls.hasFlag(KeyPressedUp)
         || joystickPointsUp
      )
      {
         auto portal = Level::getCurrentLevel()->getNearbyPortal();
         if (portal != nullptr && portal->getDestination() != nullptr)
         {
            Portal::lock();
            _portal_clock.restart();

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
   _impulse = intensity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateImpulse()
{
   if (_impulse < impulse_epsilon)
   {
      return;
   }

   auto impulse = _impulse;
   _impulse = 0.0f;

   if (GameContactListener::getInstance().isPlayerSmashed())
   {
      return;
   }

   const auto dx = _velocity_previous.x - _body->GetLinearVelocity().x;
   const auto dy = _velocity_previous.y - _body->GetLinearVelocity().y;
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

      _hard_landing = true;
      _hard_landing_cycles = 0;

      damage(static_cast<int>((impulse - 1.0f) * 20.0f));
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::damage(int32_t damage, const sf::Vector2f& force)
{
   if (isDead())
   {
      return;
   }

   if (damage == 0)
   {
      return;
   }

   if (SaveState::getPlayerInfo().mExtraTable._skills._skills & ExtraSkill::SkillInvulnerable)
   {
      return;
   }

   if (_damage_clock.getElapsedTime().asMilliseconds() > 3000)
   {
      _damage_initialized = true;

      Audio::getInstance()->playSample("hurt.wav");

      // not converting this to PPM to make the effect of the applied force more visible
      auto body = getBody();
      body->ApplyLinearImpulse(b2Vec2(force.x / PPM, force.y / PPM), body->GetWorldCenter(), true);

      SaveState::getPlayerInfo().mExtraTable._health._health -= damage;
      _damage_clock.restart();

      if (SaveState::getPlayerInfo().mExtraTable._health._health < 0)
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
      GameContactListener::getInstance().getMovingPlatformContactCount() > 0 && isOnGround();

   return onPlatform;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnGround() const
{
   return GameContactListener::getInstance().getPlayerFootContactCount() > 0;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePlatformMovement(const sf::Time& dt)
{
   if (_jump.isJumping())
   {
      return;
   }

   if (isOnPlatform() && _platform_body)
   {
      const auto dx = dt.asSeconds() * getPlatformBody()->GetLinearVelocity().x;

      const auto x = _body->GetPosition().x + dx * 1.65f;
      const auto y = _body->GetPosition().y;

      _body->SetTransform(b2Vec2(x, y), 0.0f);

      // printf("standing on platform, x: %f, y: %f, dx: %f \n", x, y, dx);
   }
}



//----------------------------------------------------------------------------------------------------------------------
void Player::updateFire()
{
   if (_controls.isFireButtonPressed())
   {
      fire();
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isInWater() const
{
   return _in_water;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setInWater(bool inWater)
{
   _in_water = inWater;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateFootsteps()
{
   if (GameContactListener::getInstance().getPlayerFootContactCount() > 0 && !isInWater())
   {
      auto vel = fabs(_body->GetLinearVelocity().x);
      if (vel > 0.1f)
      {
         if (vel < 3.0f)
            vel = 3.0f;

         if (_time.asSeconds() > _next_footstep_time)
         {
            // play footstep
            Audio::getInstance()->playSample("footstep.wav", 0.05f);
            _next_footstep_time = _time.asSeconds() + 1.0f / vel;
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
int Player::getId() const
{
   return _id;
}


//----------------------------------------------------------------------------------------------------------------------
int Player::getZIndex() const
{
   return _z_index;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setZIndex(int32_t z)
{
   _z_index = z;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateBendDown()
{
   auto downPressed = false;

   // disable bend down states when player hit dash button
   if (_dash.isDashActive())
   {
      _bend._was_bending_down = false;
      _bend._bending_down = false;
      return;
   }

   if (_controls.isControllerUsed())
   {
      const auto& joystickInfo = _controls.getJoystickInfo();
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
      downPressed = _controls.hasFlag(KeyPressedDown);
   }

   // if the head touches something while crouches, keep crouching
   if (_bend._bending_down && !downPressed && (GameContactListener::getInstance().getPlayerHeadContactCount() > 0))
   {
      return;
   }

   if (!_bend._bending_down && CameraPane::getInstance().isLookActive())
   {
      return;
   }

   const auto bending_down = downPressed && !isInAir();

   _bend._was_bending_down = _bend._bending_down;
   _bend._bending_down = bending_down;

   if (!_bend._was_bending_down && _bend._bending_down)
   {
      _bend._timepoint_bend_down_start = StopWatch::getInstance().now();
   }

   if (_bend._was_bending_down && !_bend._bending_down)
   {
      _bend._timepoint_bend_down_end = StopWatch::getInstance().now();
   }

   setMaskBitsCrouching(bending_down);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateHardLanding()
{
   {
      if (_hard_landing)
      {
         _hard_landing_cycles++;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateGroundAngle()
{
   if (!isOnGround())
   {
      _ground_normal.Set(0.0f, -1.0f);
      return;
   }

   // raycast down to determine terrain slope
   b2RayCastInput input;
   input.p1 = _body->GetPosition();
   input.p2 = _body->GetPosition() + b2Vec2(0.0f, 1.0f);
   input.maxFraction = 1.0f;

   float closestFraction = 1.0f;
   b2Vec2 intersectionNormal(0.0f, -1.0f);

   // for (b2Body* b = mWorld->GetBodyList(); b; b = b->GetNext())

   if (!_ground_body)
   {
      _ground_normal.Set(0.0f, -1.0f);
      return;
   }

   for (b2Fixture* f = _ground_body->GetFixtureList(); f; f = f->GetNext())
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

   _ground_normal = intersectionNormal;

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
   _time += dt;

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
   info._crouching = _bend.isCrouching();
   info._climbing = _climb.isClimbing();
   _jump.update(info, _controls);

   updateDash();
   _climb.update(_body, _controls, isInAir());
   updatePlatformMovement(dt);
   updatePixelPosition();
   updateFootsteps();
   updatePortal();
   updatePreviousBodyState();
   updateWeapons(dt);
   _controls.update(dt); // called at last just to backup previous controls
}


//----------------------------------------------------------------------------------------------------------------------
void Player::resetDash()
{
   // clear motion blur buffer
   _last_animations.clear();

   _player_animation.resetAlpha();

   // re-enabled gravity for player
   if (_body)
   {
      _body->SetGravityScale(1.0f);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateDash(Dash dir)
{
   if (!(SaveState::getPlayerInfo().mExtraTable._skills._skills & ExtraSkill::SkillDash))
   {
      return;
   }

   // don't allow a new dash move inside water
   if (isInWater())
   {
      if (!_dash.isDashActive())
      {
         resetDash();
         return;
      }
   }

   // dir is the initial dir passed in on button press
   // Dash::None is passed in on regular updates after the initial press

   if (dir == Dash::None)
   {
      dir = _dash._dash_dir;
   }
   else
   {
      // prevent dash spam
      if (_dash.isDashActive())
      {
         return;
      }

      _dash._dash_frame_count = PhysicsConfiguration::getInstance()._player_dash_frame_count;
      _dash._dash_multiplier = PhysicsConfiguration::getInstance()._player_dash_multiplier;
      _dash._dash_dir = dir;

#ifndef JUMP_GRAVITY_SCALING
      // disable gravity for player while dash is active
      // but only do this if gravity scaling is not used
      _body->SetGravityScale(0.0f);
      _dashSteps = 30; // hardcoded to keep it working
#endif
   }

   if (!_dash.isDashActive() || _dash._dash_dir == Dash::None)
   {
      return;
   }

   auto left = (dir == Dash::Left);
   _points_to_left = (left);

   _dash._dash_multiplier += PhysicsConfiguration::getInstance()._player_dash_multiplier_increment_per_frame;
   _dash._dash_multiplier *=PhysicsConfiguration::getInstance()._player_dash_multiplier_scale_per_frame;

   auto dashVector = _dash._dash_multiplier * _body->GetMass() * PhysicsConfiguration::getInstance()._player_dash_vector;
   auto impulse = (left) ? -dashVector : dashVector;

   _body->ApplyForceToCenter(b2Vec2(impulse, 0.0f), false);

   _dash._dash_frame_count--;

   if (!_dash.isDashActive())
   {
      resetDash();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePixelCollisions()
{
   const auto rect = getPlayerPixelRect();
   _extra_manager->collide(rect);
   Laser::collide(rect);
   Fan::collide(rect, _body);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateAtmosphere()
{
   bool wasInwater = isInWater();

   b2Vec2 pos = _body->GetPosition();
   AtmosphereTile tile = Level::getCurrentLevel()->getPhysics().getTileForPosition(pos);

   bool inWater = tile >= AtmosphereTileWaterFull && tile <= AtmosphereTileWaterCornerTopLeft;
   setInWater(inWater);

#ifdef JUMP_GRAVITY_SCALING
   // entering water
   if (inWater && !wasInwater)
   {
      _body->SetGravityScale(0.5f);
   }

   // leaving water
   if (!inWater && wasInwater)
   {
      _body->SetGravityScale(1.0f);
   }
#else
   _body->SetGravityScale(inWater ? 0.5f : 1.0f);
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
      _body->SetLinearVelocity(b2Vec2(0,0));
   }
}


//----------------------------------------------------------------------------------------------------------------------
b2Body* Player::getPlatformBody() const
{
   return _platform_body;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setPlatformBody(b2Body* body)
{
   _platform_body = body;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setGroundBody(b2Body* body)
{
   _ground_body = body;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::getVisible() const
{
   return _visible;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setVisible(bool visible)
{
   _visible = visible;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setFriction(float friction)
{
   for (b2Fixture* fixture = _body->GetFixtureList(); fixture; fixture = fixture->GetNext())
   {
      fixture->SetFriction(friction);
   }

   for (auto contact = _body->GetContactList(); contact; contact = contact->next)
   {
      contact->contact->ResetFriction();
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isInAir() const
{
   return (GameContactListener::getInstance().getPlayerFootContactCount() == 0) && !isInWater();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::fire()
{
   if (!_weapon_system->_selected)
   {
      return;
   }

   b2Vec2 dir;

   dir.x =
      _points_to_left
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

   pos.x = xOffset + _pixel_position_f.x * MPP;
   pos.y = yOffset + _pixel_position_f.y * MPP;

   _weapon_system->_selected->fireInIntervals(
      _world,
      pos,
      dir
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateDeadFixtures()
{
   if (!_body_fixture)
   {
      return;
   }

   for (int32_t i = 0; i < __foot_count; i++)
   {
      _foot_fixture[i]->SetSensor(_dead);
   }

   _body_fixture->SetSensor(_dead);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateWeapons(const sf::Time& dt)
{
   for (auto& w : _weapon_system->_weapons)
   {
      w->update(dt);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::die()
{
   _dead = true;

   updateDeadFixtures();

   Audio::getInstance()->playSample("death.wav");
}


//----------------------------------------------------------------------------------------------------------------------
void Player::reset()
{
   // check for checkpoints
   // so start position could vary here
   _hard_landing = false;
   _hard_landing_cycles = 0;

   if (_body)
   {
      _body->SetLinearVelocity(b2Vec2(0,0));
      _body->SetGravityScale(1.0);
   }

   _climb.removeClimbJoint();

   if (Level::getCurrentLevel())
   {
      setBodyViaPixelPosition(
         Level::getCurrentLevel()->getStartPosition().x,
         Level::getCurrentLevel()->getStartPosition().y
      );
   }

   SaveState::getPlayerInfo().mExtraTable._health.reset();

   // resetting any player info apart form the health doesn't make sense
   // since it's loaded from disk when the player dies
   // SaveState::getPlayerInfo().mInventory.resetKeys();

   // reset bodies passed from the contact listener
   _platform_body = nullptr;
   _ground_body = nullptr;

   // reset dash
   _dash._dash_frame_count = 0;
   resetDash();
   _dead = false;

   // fixtures are no longer dead
   updateDeadFixtures();
}


//----------------------------------------------------------------------------------------------------------------------
DeathReason Player::checkDead() const
{
   DeathReason reason = DeathReason::None;

   const auto touches_something_deadly = (GameContactListener::getInstance().getDeadlyContactCount() > 0);
   const auto too_fast = fabs(_body->GetLinearVelocity().y) > 40;
   const auto out_of_health = SaveState::getPlayerInfo().mExtraTable._health._health <= 0;
   const auto smashed = GameContactListener::getInstance().isPlayerSmashed();

   if (touches_something_deadly)
   {
      reason = DeathReason::TouchesDeadly;
   }
   else if (smashed)
   {
      reason = DeathReason::Smashed;
   }
   else if (too_fast)
   {
      reason = DeathReason::TooFast;
   }
   else if (out_of_health)
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
   return _body->GetPosition();
}


//----------------------------------------------------------------------------------------------------------------------
void Player::traceJumpCurve()
{
   if (_controls.isJumpButtonPressed())
   {
      if (!_jump_trace.jumpStarted)
      {
         _jump_trace.jumpStartTime = _time;
         _jump_trace.jumpStartY = _body->GetPosition().y;
         _jump_trace.jumpStarted = true;
         std::cout << std::endl << "time; y" << std::endl;
      }

      const auto jumpNextY = -(_body->GetPosition().y - _jump_trace.jumpStartY);
      if (fabs(jumpNextY - _jump_trace.jumpPrevY) > _jump_trace.jumpEpsilon)
      {
         std::cout
            << _time.asSeconds() - _jump_trace.jumpStartTime.asSeconds()
            << "; "
            << jumpNextY
            << std::endl;
      }

      _jump_trace.jumpPrevY = jumpNextY;
   }
   else
   {
      _jump_trace.jumpStarted = false;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::keyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Space)
   {
      _jump.jump();
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
   return _weapon_system;
}


//----------------------------------------------------------------------------------------------------------------------
PlayerControls& Player::getControls()
{
   return _controls;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePixelPosition()
{
   // sync player sprite with with box2d data
   float x = _body->GetPosition().x * PPM;
   float y = _body->GetPosition().y * PPM;

   // traceJumpCurve();

   setPixelPosition(x, y);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updatePreviousBodyState()
{
   _position_previous = _body->GetPosition();
   _velocity_previous = _body->GetLinearVelocity();
}


