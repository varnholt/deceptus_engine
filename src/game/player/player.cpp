#include "player.h"

#include "animationpool.h"
#include "audio.h"
#include "bow.h"
#include "camerapane.h"
#include "displaymode.h"
#include "gameclock.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "fadetransitioneffect.h"
#include "framework/tools/stopwatch.h"
#include "fixturenode.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gun.h"
#include "level.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "onewaywall.h"
#include "physics/physicsconfiguration.h"
#include "playerinfo.h"
#include "savestate.h"
#include "screentransition.h"
#include "sword.h"
#include "texturepool.h"
#include "tweaks.h"
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
void Player::setBeltVelocity(float belt_velocity)
{
  _belt_velocity = belt_velocity;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isOnBelt() const
{
  return _is_on_belt;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::setOnBelt(bool on_belt)
{
  _is_on_belt = on_belt;
}


//----------------------------------------------------------------------------------------------------------------------
Player::Player(GameNode* parent)
  : GameNode(parent)
{
   setName(typeid(Player).name());

   __current = this;

   _weapon_system = std::make_shared<WeaponSystem>();
   _extra_manager = std::make_shared<ExtraManager>();
   _controls = std::make_shared<PlayerControls>();

   _climb.setControls(_controls);
   _jump.setControls(_controls);
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

   _jump._dust_animation_callback = [this](){
      AnimationPool::getInstance().add(
         _points_to_left
          ? "player_jump_dust_l"
          : "player_jump_dust_r",
         _pixel_position_f.x,
         _pixel_position_f.y
      );
   };

   _jump._remove_climb_joint_callback = [this](){
         _climb.removeClimbJoint();
      };

   _controls->addKeypressedCallback([this](sf::Keyboard::Key key){keyPressed(key);});

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
   auto& gji = GameControllerIntegration::getInstance();

   gji.addDeviceAddedCallback([&](int32_t /*id*/){
         auto is_running = []() -> bool {
            return (GameState::getInstance().getMode() == ExecutionMode::Running);
         };

         auto toggle_mechanism = [](){
            Level::getCurrentLevel()->toggleMechanisms();
         };

         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, [&](){if (is_running()){_jump.jump();}});
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_X, [&](){if (is_running()){toggle_mechanism();}});
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, [&](){if (is_running()){updateDash(Dash::Left);}});
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, [&](){if (is_running()){updateDash(Dash::Right);}});
      }
   );
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
         anim._animation->setPosition(anim._position);
         anim._animation->setAlpha(static_cast<uint8_t>(255/(2*(_last_animations.size()-i))));
         anim._animation->draw(color);
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
      {
         "player_jump_dust_l",
         "player_jump_dust_r",
         "player_water_splash"
      }
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
void Player::updatePixelRect()
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
   const auto& axis_values = _controls->getJoystickInfo().getAxisValues();

   if (_controls->isLookingAround())
   {
      return 0.0f;
   }

   // analogue input normalized to -1..1
   const auto axis_value = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   auto axis_value_normalized = axis_values[static_cast<size_t>(axis_value)] / 32767.0f;

   // digital input
   const auto hat_value = _controls->getJoystickInfo().getHatValues().at(0);
   const auto dpad_left_pressed  = hat_value & SDL_HAT_LEFT;
   const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

   if (dpad_left_pressed)
   {
      axis_value_normalized = -1.0f;
   }
   else if (dpad_right_pressed)
   {
      axis_value_normalized = 1.0f;
   }

   // controller is not used, so slow down
   if (fabs(axis_value_normalized) <= 0.3f)
   {
      return speed.current_velocity.x * speed._deceleration;
   }

   axis_value_normalized *= speed._acceleration;

   // checking for the current speed here because even if the player pushes a controller axis
   // to the left side, it might still dash to the other side with quite a strong impulse.
   // that would confuse the speed capping and would accelerate to infinity. true story.
   auto desired_velocity = 0.0f;
   if (speed.current_velocity.x < 0.0f)
   {
      desired_velocity = b2Max(speed.current_velocity.x + axis_value_normalized, -speed._velocity_max);

   }
   else
   {
      desired_velocity = b2Min(speed.current_velocity.x + axis_value_normalized, speed._velocity_max);
   }

   // Log::Info()
   //    << "desired: " << desiredVel << " "
   //    << "current: " << speed.currentVelocity.x << " "
   //    << "axis value: " << axisValueNormalized << " "
   //    << "max: " << speed.velocityMax;

   return desired_velocity;
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
void Player::updateOrientation()
{
   if (isDead())
   {
      return;
   }

   const auto orientation = _controls->getActiveOrientation();
   if (orientation == PlayerControls::Orientation::Left)
   {
      _points_to_left = true;
   }
   else if (orientation == PlayerControls::Orientation::Right)
   {
      _points_to_left = false;
   }
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getVelocityFromKeyboard(const PlayerSpeed& speed) const
{
   if (_controls->hasFlag(KeyPressedLook))
   {
      return 0.0f;
   }

   // sanity check to avoid moonwalking
   if (_controls->hasFlag(KeyPressedLeft) && _controls->hasFlag(KeyPressedRight))
   {
      return 0.0f;
   }

   float desiredVel = 0.0f;

   if (_controls->hasFlag(KeyPressedLeft))
   {
      desiredVel = b2Max(speed.current_velocity.x - speed._acceleration, -speed._velocity_max);
   }

   if (_controls->hasFlag(KeyPressedRight))
   {
      desiredVel = b2Min(speed.current_velocity.x + speed._acceleration, speed._velocity_max);
   }

   // slowdown as soon as
   // a) no movement to left or right
   // b) movement is opposite to given direction
   // c) no movement at all
   const auto noMovementToLeftOrRight =
         (!(_controls->hasFlag(KeyPressedLeft)))
      && (!(_controls->hasFlag(KeyPressedRight)));

   const auto velocityOppositeToGivenDir =
         (speed.current_velocity.x < -0.01f && _controls->hasFlag(KeyPressedRight))
      || (speed.current_velocity.x >  0.01f && _controls->hasFlag(KeyPressedLeft));

   const auto noMovement = (fabs(desiredVel) < 0.0001f);

   if (noMovementToLeftOrRight || velocityOppositeToGivenDir || noMovement)
   {
      desiredVel = speed.current_velocity.x * speed._deceleration;
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
bool Player::isDead() const
{
   return _dead;
}


//----------------------------------------------------------------------------------------------------------------------
bool Player::isJumpingThroughOneWayWall()
{
   // a player is considered jumping through a one-way wall when
   // - the y velocity goes up
   // - there are active contacts with a one-way wall
   if (_body->GetLinearVelocity().y < 0.0f)
   {
      if (OneWayWall::instance().hasContacts())
      {
         return true;
      }
   }

   return false;
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
   data._moving_left = _controls->isMovingLeft();
   data._moving_right = _controls->isMovingRight();
   data._wall_sliding = _jump._wallsliding;
   data._wall_jump_points_right = _jump._walljump_points_right;
   data._jumping_through_one_way_wall = isJumpingThroughOneWayWall();
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
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity() const
{
   const auto acceleration = getAcceleration();
   const auto deceleration = getDeceleration();

   const auto current_velocity = _body->GetLinearVelocity();
   const auto velocity_max = getMaxVelocity();

   PlayerSpeed speed
   {
      current_velocity,
      velocity_max,
      acceleration,
      deceleration
   };

   const auto desired_velocity = getDesiredVelocity(speed);
   return desired_velocity;
}


//----------------------------------------------------------------------------------------------------------------------
float Player::getDesiredVelocity(const PlayerSpeed& speed) const
{
  auto desired_velocity = 0.0f;

  if (GameControllerIntegration::getInstance().isControllerConnected())
  {
     if (_controls->isControllerUsedLast())
     {
        desired_velocity = getVelocityFromController(speed);
     }
     else
     {
        desired_velocity = getVelocityFromKeyboard(speed);
     }
  }
  else
  {
     desired_velocity = getVelocityFromKeyboard(speed);
  }

  return desired_velocity;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::applyBeltVelocity(float& desired_velocity)
{
   if (isOnBelt())
   {
      if (getBeltVelocity() < 0.0f)
      {
         if (_controls->isMovingRight())
         {
            desired_velocity *= 0.5f;
         }
         else if (_controls->isMovingLeft())
         {
            if (desired_velocity > 0.0f)
            {
               desired_velocity = 0.0f;
            }

            desired_velocity *= 2.0f;
            desired_velocity = std::min(desired_velocity, getMaxVelocity());
         }
         else
         {
            desired_velocity += getBeltVelocity();
         }
      }
      else if (getBeltVelocity() > 0.0f)
      {
         if (_controls->isMovingLeft())
         {
            desired_velocity *= 0.5f;
         }
         else if (_controls->isMovingRight())
         {
            if (desired_velocity < 0.0f)
            {
               desired_velocity = 0.0f;
            }

            desired_velocity *= 2.0f;
            desired_velocity = std::max(desired_velocity, -getMaxVelocity());
         }
         else
         {
            desired_velocity += getBeltVelocity();
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateVelocity()
{
   using namespace std::chrono_literals;

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

   if (ScreenTransitionHandler::getInstance().active())
   {
      const auto velocity = _body->GetLinearVelocity();
      _body->SetLinearVelocity(b2Vec2{0.0, velocity.y});
      return;
   }

   // if we just landed hard on the ground, we need a break :)
   if (_hard_landing)
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (GameClock::getInstance().duration() < _player_animation.getRevealDuration())
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (_bend._bending_down)
   {
      if (!(SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(ExtraSkill::Skill::Crouch)))
      {
         if (getControls()->isDroppingDown() && OneWayWall::instance().hasContacts())
         {
            // usually just stop the player from movement when bending down while he has no crouching ability
            // however, when dropping from a platform, we don't want to mess with the velocity,
            // just let the player fall
         }
         else
         {
            const auto velocity = _body->GetLinearVelocity();
            _body->SetLinearVelocity(b2Vec2{0.0, velocity.y});
            return;
         }
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

   auto desired_velocity = getDesiredVelocity();
   auto current_velocity = _body->GetLinearVelocity();

   // physically so wrong but gameplay-wise the best choice :)
   applyBeltVelocity(desired_velocity);

   // calc impulse, disregard time factor
   auto velocity_change_x = desired_velocity - current_velocity.x;
   auto impulse_x = _body->GetMass() * velocity_change_x;

   _body->ApplyLinearImpulse(
      b2Vec2(impulse_x, 0.0f),
      _body->GetWorldCenter(),
      true
   );

   // TODO: port this to box2d buoyancy: http://www.iforce2d.net/b2dtut/buoyancy
   // limit velocity
   if (isInWater())
   {
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
   static const auto max_speed = 10.0f;
   b2Vec2 vel = _body->GetLinearVelocity();
   const auto speed = vel.Normalize();
   if (speed > max_speed)
   {
      // Log::Info() << "cap speed";
      _body->SetLinearVelocity(max_speed * vel);
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<ScreenTransition> Player::makeFadeTransition()
{
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

   return std::move(screen_transition);
}


//----------------------------------------------------------------------------------------------------------------------
void Player::goToPortal(auto portal)
{
   auto dst_position_px =  portal->getDestination()->getPortalPosition();

   setBodyViaPixelPosition(
      dst_position_px.x + PLAYER_ACTUAL_WIDTH / 2,
      dst_position_px.y + DIFF_PLAYER_TILE_TO_PHYSICS
   );

   // update the camera system to point to the player position immediately
   CameraSystem::getCameraSystem().syncNow();
   Portal::unlock();
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
      const auto& joystick_info = _controls->getJoystickInfo();
      const auto& axis_values = joystick_info.getAxisValues();
      auto joystick_points_up = false;

      if (!axis_values.empty())
      {
         auto dpad_up_pressed = false;
         if (!joystick_info.getHatValues().empty())
         {
            dpad_up_pressed = joystick_info.getHatValues().at(0) & SDL_HAT_UP;
         }

         auto y1 = axis_values[1] / 32767.0f;
         joystick_points_up = (y1 < Tweaks::instance()._enter_portal_threshold) || dpad_up_pressed;
      }

      if (
            _controls->hasFlag(KeyPressedUp)
         || joystick_points_up
      )
      {
         auto portal = Level::getCurrentLevel()->getNearbyPortal();
         if (portal != nullptr && portal->getDestination() != nullptr)
         {
            Portal::lock();
            _portal_clock.restart();

            auto screen_transition = makeFadeTransition();
            screen_transition->_callbacks_effect_1_ended.push_back([this, portal](){goToPortal(portal);});
            screen_transition->_callbacks_effect_2_ended.push_back([](){ScreenTransitionHandler::getInstance().pop();});
            ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
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

   // Log::Info()
   //    << "intensity: " << intensity
   //    << " dx: " << dx
   //    << " dy: " << dy
   //    << " dir: " << (horizontal ? "x" : "y");

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

      _timepoint_hard_landing = StopWatch::getInstance().now();
      _hard_landing = true;
      _hard_landing_cycles = 0;

      if (PhysicsConfiguration::getInstance()._player_hard_landing_damage_enabled)
      {
         damage(static_cast<int32_t>((impulse - 1.0f) * PhysicsConfiguration::getInstance()._player_hard_landing_damage_factor));
      }
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

   if (SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(ExtraSkill::Skill::Invulnerable))
   {
      return;
   }

   if (_damage_clock.getElapsedTime().asMilliseconds() > 3000)
   {
      _damage_initialized = true;

      Audio::getInstance().playSample("hurt.wav");

      // not converting this to PPM to make the effect of the applied force more visible
      auto body = getBody();
      body->ApplyLinearImpulse(b2Vec2(force.x / PPM, force.y / PPM), body->GetWorldCenter(), true);

      SaveState::getPlayerInfo()._extra_table._health._health -= damage;
      _damage_clock.restart();

      if (SaveState::getPlayerInfo()._extra_table._health._health < 0)
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
   const auto on_platform =
      GameContactListener::getInstance().getMovingPlatformContactCount() > 0 && isOnGround();

   return on_platform;
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
   if (_controls->isFireButtonPressed())
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
            Audio::getInstance().playSample("footstep.wav", 0.05f);
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
   // disable bend down states when player hit dash button
   if (_dash.isDashActive())
   {
      _bend._was_bending_down = false;
      _bend._bending_down = false;
      return;
   }

   auto down_pressed = _controls->isBendDownActive();

   // if the head touches something while crouches, keep crouching
   if (_bend._bending_down && !down_pressed && (GameContactListener::getInstance().getPlayerHeadContactCount() > 0))
   {
      return;
   }

   if (!_bend._bending_down && CameraPane::getInstance().isLookActive())
   {
      return;
   }

   const auto bending_down = down_pressed && !isInAir();

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
   using namespace std::chrono_literals;

   if (_hard_landing)
   {
      _hard_landing_cycles++;

      // this should be longer and player should go into bend down position after a hard landing
      if (StopWatch::getInstance().now() - _timepoint_hard_landing > 0.2s)
      {
         _hard_landing = false;
         _hard_landing_cycles = 0;
      }
   }

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

      // Log::Info() << "hard landing: " << mHardLanding << " on ground: " << isOnGround() << " on platform: "<< isOnPlatform();
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

   for (auto f = _ground_body->GetFixtureList(); f; f = f->GetNext())
   {
      // terrain is made out of chains, so only process those
      if (f->GetShape()->GetType() != b2Shape::e_chain)
      {
         continue;
      }

      b2RayCastOutput output;

      for (auto child_index = 0; child_index < f->GetShape()->GetChildCount(); child_index++)
      {
         if (!f->RayCast(&output, input, child_index))
            continue;

         if (output.fraction < closestFraction)
         {
            closestFraction = output.fraction;
            intersectionNormal = output.normal;
         }
      }
   }

   _ground_normal = intersectionNormal;
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateOneWayWallDrop()
{
   if (getControls()->isDroppingDown())
   {
      OneWayWall::instance().drop();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::update(const sf::Time& dt)
{
   _time += dt;

   updateImpulse();
   updateGroundAngle();
   updateHardLanding();
   updatePixelRect();
   updateBendDown();
   updateAnimation(dt);
   updatePixelCollisions();
   updateAtmosphere();
   updateFire();
   updateVelocity();
   updateOrientation();
   updateOneWayWallDrop();

   PlayerJump::PlayerJumpInfo info;
   info._in_air = isInAir();
   info._in_water = isInWater();
   info._crouching = _bend.isCrouching();
   info._climbing = _climb.isClimbing();
   _jump.update(info);

   updateDash();
   _climb.update(_body, isInAir());
   updatePlatformMovement(dt);
   updatePixelPosition();
   updateFootsteps();
   updatePortal();
   updatePreviousBodyState();
   updateWeapons(dt);
   _controls->update(dt); // called at last just to backup previous controls
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
   if (!(SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(ExtraSkill::Skill::Dash)))
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
   AtmosphereTile tile = Level::getCurrentLevel()->getAtmosphere().getTileForPosition(pos);

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
      Audio::getInstance().playSample("splash.wav");
      // https://freesound.org/people/Rocktopus/packs/14347/

      AnimationPool::getInstance().add(
          "player_water_splash",
         _pixel_position_f.x,
         _pixel_position_f.y
      );
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

   b2Vec2 pos;
   b2Vec2 dir;
   dir.x = _points_to_left ? -1.0f : 1.0f;
   dir.y = 0.0f;

   switch (_weapon_system->_selected->getWeaponType())
   {
      case WeaponType::Bow:
      {
         dir.y = -0.1f;

         constexpr auto force = 1.5f;
         const auto x_offset = dir.x * 0.5f;
         const auto y_offset = -0.25f;

         pos.x = x_offset + _pixel_position_f.x * MPP;
         pos.y = y_offset + _pixel_position_f.y * MPP;

         dynamic_pointer_cast<Bow>(_weapon_system->_selected)->useInIntervals(_world, pos, force * dir);
         break;
      }
      case WeaponType::Gun:
      {
         constexpr auto force = 10.0f;
         const auto x_offset = dir.x * 1.0f;
         const auto y_offset = -0.1f;

         pos.x = x_offset + _pixel_position_f.x * MPP;
         pos.y = y_offset + _pixel_position_f.y * MPP;

         dynamic_pointer_cast<Gun>(_weapon_system->_selected)->useInIntervals(_world, pos, force * dir);
         break;
      }
      case WeaponType::Sword:
      {
         const auto x_offset = dir.x * 0.5f;
         const auto y_offset = 0.0f;

         const auto player_width_px = _pixel_rect.width / 2;

         pos.x = x_offset + _pixel_position_f.x * MPP  - player_width_px * MPP;
         pos.y = y_offset + _pixel_position_f.y * MPP;

         dynamic_pointer_cast<Sword>(_weapon_system->_selected)->use(_world, pos, dir);
         break;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Player::updateDeadFixtures()
{
   return;

   // disable all collision control for the player. this has been useful before
   // but now can be removed permanently.
   //
   // if (!_body_fixture)
   // {
   //    return;
   // }
   //
   // for (int32_t i = 0; i < __foot_count; i++)
   // {
   //    _foot_fixture[i]->SetSensor(_dead);
   // }
   //
   // _body_fixture->SetSensor(_dead);
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

   Audio::getInstance().playSample("death.wav");
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

   SaveState::getPlayerInfo()._extra_table._health.reset();

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
   const auto out_of_health = SaveState::getPlayerInfo()._extra_table._health._health <= 0;
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
void Player::traceJumpCurve()
{
   if (_controls->isJumpButtonPressed())
   {
      if (!_jump_trace._jump_started)
      {
         _jump_trace._jump_start_time = _time;
         _jump_trace._jump_start_y = _body->GetPosition().y;
         _jump_trace._jump_started = true;

         std::cout << std::endl << "time; y" << std::endl;
      }

      const auto jumpNextY = -(_body->GetPosition().y - _jump_trace._jump_start_y);
      if (fabs(jumpNextY - _jump_trace._jump_prev_y) > _jump_trace._jump_epsilon)
      {
         std::cout
            << _time.asSeconds() - _jump_trace._jump_start_time.asSeconds()
            << "; "
            << jumpNextY
            << std::endl;
      }

      _jump_trace._jump_prev_y = jumpNextY;
   }
   else
   {
      _jump_trace._jump_started = false;
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
const std::shared_ptr<PlayerControls>& Player::getControls()
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


