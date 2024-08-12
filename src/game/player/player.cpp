#include "player.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "framework/tools/stopwatch.h"
#include "game/audio/audio.h"
#include "game/camera/camerapanorama.h"
#include "game/clock/gameclock.h"
#include "game/config/gameconfiguration.h"
#include "game/config/tweaks.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/screentransition.h"
#include "game/level/fixturenode.h"
#include "game/level/level.h"
#include "game/mechanisms/bouncerwrapper.h"
#include "game/mechanisms/fan.h"
#include "game/mechanisms/laser.h"
#include "game/mechanisms/portalwrapper.h"
#include "game/physics/chainshapeanalyzer.h"
#include "game/physics/gamecontactlistener.h"
#include "game/physics/onewaywall.h"
#include "game/physics/physicsconfiguration.h"
#include "game/player/inventorybasedcontrols.h"
#include "game/player/playeraudio.h"
#include "game/player/playercontrolstate.h"
#include "game/player/playerinfo.h"
#include "game/player/weaponsystem.h"
#include "game/state/savestate.h"
#include "game/weapons/weapon.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

namespace
{
constexpr uint16_t category_bits = CategoryFriendly;
constexpr uint16_t mask_bits_standing = CategoryBoundary | CategoryEnemyCollideWith;
constexpr uint16_t mask_bits_crouching = CategoryEnemyCollideWith;
constexpr int16_t group_index = 0;
constexpr auto impulse_epsilon = 0.0000001f;

constexpr auto wall_slide_sensor_width = 8.0f;
constexpr auto wall_slide_sensor_height = 0.75f;
constexpr auto wall_slide_sensor_distance = 0.21f;
}  // namespace

Player* Player::__current = nullptr;

b2Body* Player::getBody() const
{
   return _body;
}

b2Fixture* Player::getFootSensorFixture() const
{
   return _foot_sensor_fixture;
}

sf::IntRect Player::computeFootSensorPixelIntRect() const
{
   sf::IntRect rect_px;
   b2AABB aabb;

   _foot_sensor_fixture->GetShape()->ComputeAABB(&aabb, _body->GetTransform(), 0);

   rect_px.left = static_cast<int32_t>(aabb.lowerBound.x * PPM);
   rect_px.top = static_cast<int32_t>(aabb.lowerBound.y * PPM);
   rect_px.width = static_cast<int32_t>(abs(aabb.upperBound.x - aabb.lowerBound.x) * PPM);
   rect_px.height = static_cast<int32_t>(abs(aabb.upperBound.y - aabb.lowerBound.y) * PPM);

   // std::cout
   //    << "ux: " << aabb.upperBound.x << " "
   //    << "uy: " << aabb.upperBound.y << " "
   //    << "lx: " << aabb.lowerBound.x << " "
   //    << "ly: " << aabb.lowerBound.y << " "
   //    << std::endl;

   return rect_px;
}

sf::FloatRect Player::computeFootSensorPixelFloatRect() const
{
   sf::FloatRect rect_px;
   b2AABB aabb;

   _foot_sensor_fixture->GetShape()->ComputeAABB(&aabb, _body->GetTransform(), 0);

   rect_px.left = aabb.lowerBound.x * PPM;
   rect_px.top = aabb.lowerBound.y * PPM;
   rect_px.width = abs(aabb.upperBound.x - aabb.lowerBound.x) * PPM;
   rect_px.height = abs(aabb.upperBound.y - aabb.lowerBound.y) * PPM;

   // std::cout
   //    << "ux: " << aabb.upperBound.x << " "
   //    << "uy: " << aabb.upperBound.y << " "
   //    << "lx: " << aabb.lowerBound.x << " "
   //    << "ly: " << aabb.lowerBound.y << " "
   //    << std::endl;

   return rect_px;
}

Player::Player(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Player).name());

   __current = this;

   PlayerAudio::addSamples();

   _controls = std::make_shared<PlayerControls>();
   _player_animation = std::make_shared<PlayerAnimation>();

   _climb.setControls(_controls);
   _jump.setControls(_controls);

   _dash._reset_dash_callback = [this]() { resetMotionBlur(); };

   _player_animation->loadAnimations(_animation_pool);
}

Player* Player::getCurrent()
{
   return __current;
}

void Player::initialize()
{
   _portal_clock.restart();
   _damage_clock.restart();

   _jump._jump_dust_animation_callback = [this](PlayerJump::DustAnimationType animation_type)
   {
      switch (animation_type)
      {
         case PlayerJump::DustAnimationType::Ground:
         {
            _animation_pool.create(_points_to_left ? "player_jump_dust_l" : "player_jump_dust_r", _pixel_position_f.x, _pixel_position_f.y);
            break;
         }
         case PlayerJump::DustAnimationType::InAir:
         {
            _animation_pool.create(
               _points_to_left ? "player_jump_dust_inair_l" : "player_jump_dust_inair_r", _pixel_position_f.x, _pixel_position_f.y
            );
            break;
         }
      }
   };

   _jump._remove_climb_joint_callback = [this]() { _climb.removeClimbJoint(); };

   _controls->addKeypressedCallback([this](sf::Keyboard::Key key) { keyPressed(key); });

   initializeController();
}

void Player::initializeLevel()
{
   createPlayerBody();

   setBodyViaPixelPosition(Level::getCurrentLevel()->getStartPosition().x, Level::getCurrentLevel()->getStartPosition().y);
}

void Player::initializeController()
{
   auto& gji = GameControllerIntegration::getInstance();

   gji.addDeviceAddedCallback(
      [&](int32_t /*id*/)
      {
         gji.getController()->addButtonPressedCallback(
            SDL_CONTROLLER_BUTTON_A,
            [&]()
            {
               if (!PlayerControlState::checkState())
               {
                  return;
               }

               _jump.jump();
            }
         );

         gji.getController()->addButtonPressedCallback(
            SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
            [&]()
            {
               if (!PlayerControlState::checkState())
               {
                  return;
               }

               updateDash(Dash::Left);
            }
         );

         gji.getController()->addButtonPressedCallback(
            SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            [&]()
            {
               if (!PlayerControlState::checkState())
               {
                  return;
               }

               updateDash(Dash::Right);
            }
         );

         gji.getController()->addButtonPressedCallback(
            SDL_CONTROLLER_BUTTON_X,
            [&]()
            {
               if (!PlayerControlState::checkState())
               {
                  return;
               }

               useInventory(0);
            }
         );

         gji.getController()->addButtonPressedCallback(
            SDL_CONTROLLER_BUTTON_Y,
            [&]()
            {
               if (!PlayerControlState::checkState())
               {
                  return;
               }

               useInventory(1);
            }
         );
      }
   );
}

void Player::setBodyViaPixelPosition(float x, float y)
{
   setPixelPosition(x, y);

   if (_body)
   {
      _body->SetTransform(b2Vec2(x * MPP, y * MPP), 0.0f);
   }
}

bool Player::checkDamageDrawSkip() const
{
   if (isDead())
   {
      // dead players shouldn't flash
      return false;
   }

   auto skip_render = false;
   const auto time = GlobalClock::getInstance().getElapsedTimeInMs();
   const auto damage_time = _damage_clock.getElapsedTime().asMilliseconds();
   if (_damage_initialized && time > 3000 && damage_time < 3000)
   {
      if ((damage_time / 100) % 2 == 0)
      {
         skip_render = true;
      }
   }

   return skip_render;
}

void Player::updateHurtColor(const std::shared_ptr<Animation>& current_cycle)
{
   if (isDead())
   {
      return;
   }

   // update color if player is hurt
   constexpr auto red_intensity = 200;
   const auto damage_color_value = static_cast<uint8_t>(red_intensity * std::max(0.0f, 1.0f - _damage_clock.getElapsedTime().asSeconds()));
   if (damage_color_value > 0)
   {
      const auto damage_color = sf::Color(255, 255 - damage_color_value, 255 - damage_color_value);
      current_cycle->setColor(damage_color);
   }
}

void Player::useInventory(int32_t slot)
{
   auto& inventory = SaveState::getPlayerInfo()._inventory;
   inventory.use(slot);
}

void Player::drawDash(sf::RenderTarget& color, const std::shared_ptr<Animation>& current_cycle, const sf::Vector2f& draw_position_px)
{
   // draw dash with motion blur
   for (auto i = 0u; i < _last_animations.size(); i++)
   {
      auto& anim = _last_animations[i];
      anim._animation->setPosition(anim._position);
      anim._animation->setAlpha(static_cast<uint8_t>(255 / (2 * (_last_animations.size() - i))));
      anim._animation->draw(color);
   }

   if (_dash.hasMoreFrames())
   {
      _last_animations.push_back({draw_position_px, current_cycle});
   }
}

void Player::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   _water_bubbles.draw(color, normal);

   if (!_visible)
   {
      return;
   }

   if (checkDamageDrawSkip())
   {
      return;
   }

   if (_jump._wallsliding && _jump._walljump_frame_count == 0)
   {
      _player_animation->getWallslideAnimation()->draw(color);
   }

   const auto& weapon_system = SaveState::getPlayerInfo()._weapons;
   if (weapon_system._selected)
   {
      weapon_system._selected->draw(color);
   }

   // that y offset is to compensate the wonky box2d origin
   const auto draw_position_px = _pixel_position_f + sf::Vector2f(0, 8);

   const auto& current_cycle = _player_animation->getCurrentCycle();
   if (current_cycle)
   {
      current_cycle->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(_fade_out_alpha * 255)));
      current_cycle->setPosition(draw_position_px);
      drawDash(color, current_cycle, draw_position_px);
      updateHurtColor(current_cycle);
      current_cycle->draw(color, normal);
   }

   const auto& auxiliary_cycle = _player_animation->getAuxiliaryCycle();
   if (auxiliary_cycle)
   {
      auxiliary_cycle->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(_fade_out_alpha * 255)));
      auxiliary_cycle->setPosition(draw_position_px);
      auxiliary_cycle->draw(color, normal);
   }

   // draw additional effects such as dust, water splash
   _animation_pool.drawAnimations(
      color,
      normal,
      {"player_jump_dust_l", "player_jump_dust_r", "player_jump_dust_inair_l", "player_jump_dust_inair_r", "player_water_splash"}
   );
}

void Player::drawStencil(sf::RenderTarget& color)
{
   const auto stencil_color = sf::Color{255, 255, 255, 40};
   const auto draw_position_px = _pixel_position_f + sf::Vector2f(0, 8);

   auto current_cycle = _player_animation->getCurrentCycle();
   if (current_cycle)
   {
      current_cycle->setColor(stencil_color);
      current_cycle->setPosition(draw_position_px);
      current_cycle->draw(color);
   }

   auto auxiliary_cycle = _player_animation->getAuxiliaryCycle();
   if (auxiliary_cycle)
   {
      auxiliary_cycle->setColor(stencil_color);
      auxiliary_cycle->setPosition(draw_position_px);
      auxiliary_cycle->draw(color);
   }
}

const sf::Vector2f& Player::getPixelPositionFloat() const
{
   return _pixel_position_f;
}

const sf::Vector2i& Player::getPixelPositionInt() const
{
   return _pixel_position_i;
}

void Player::setPixelPosition(float x, float y)
{
   _pixel_position_f.x = x;
   _pixel_position_f.y = y;

   _pixel_position_i.x = static_cast<int32_t>(x);
   _pixel_position_i.y = static_cast<int32_t>(y);
}

const sf::FloatRect& Player::getPixelRectFloat() const
{
   return _pixel_rect_f;
}

void Player::updatePixelRect()
{
   constexpr auto height_diff_px = PLAYER_TILES_HEIGHT - PLAYER_ACTUAL_HEIGHT;

   _pixel_rect_f.left = _pixel_position_f.x - PLAYER_ACTUAL_WIDTH * 0.5f;
   _pixel_rect_f.top = _pixel_position_f.y - height_diff_px - (height_diff_px * 0.5f);
   _pixel_rect_f.width = PLAYER_ACTUAL_WIDTH;
   _pixel_rect_f.height = PLAYER_ACTUAL_HEIGHT;

   _pixel_rect_i.left = static_cast<int32_t>(_pixel_rect_f.left);
   _pixel_rect_i.top = static_cast<int32_t>(_pixel_rect_f.top);
   _pixel_rect_i.width = PLAYER_ACTUAL_WIDTH;
   _pixel_rect_i.height = PLAYER_ACTUAL_HEIGHT;
}

void Player::updateChunk()
{
   _chunk.update(_pixel_position_i.x, _pixel_position_i.y);
}

const sf::IntRect& Player::getPixelRectInt() const
{
   return _pixel_rect_i;
}

void Player::setMaskBitsCrouching(bool enabled)
{
   b2Filter filter = _body_fixture->GetFilterData();
   filter.maskBits = enabled ? mask_bits_crouching : mask_bits_standing;
   _body_fixture->SetFilterData(filter);
}

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

   const auto width_px = PLAYER_ACTUAL_WIDTH;
   const auto height_px = PLAYER_ACTUAL_HEIGHT;
   const auto feet_radius_m = 0.16f / static_cast<float>(__foot_count);
   const auto feet_distance_m = 0.0f;
   const auto feet_offset_m = static_cast<float>(__foot_count) * (feet_radius_m * 2.0f + feet_distance_m) * 0.5f - feet_radius_m;

   for (auto i = 0u; i < __foot_count; i++)
   {
      b2FixtureDef fixture_def_feet;
      fixture_def_feet.density = 1.f;
      fixture_def_feet.friction = PhysicsConfiguration::getInstance()._player_friction;
      fixture_def_feet.restitution = 0.0f;
      fixture_def_feet.filter.categoryBits = category_bits;
      fixture_def_feet.filter.maskBits = mask_bits_standing;
      fixture_def_feet.filter.groupIndex = group_index;

      b2CircleShape feet_shape;
      feet_shape.m_p.Set(i * (feet_radius_m * 2.0f + feet_distance_m) - feet_offset_m, 0.12f);
      feet_shape.m_radius = feet_radius_m;
      fixture_def_feet.shape = &feet_shape;

      auto* foot = _body->CreateFixture(&fixture_def_feet);
      _foot_fixture[i] = foot;

      auto* object_data_feet = new FixtureNode(this);
      object_data_feet->setType(ObjectTypePlayer);
      object_data_feet->setFlag("foot", true);
      foot->SetUserData(static_cast<void*>(object_data_feet));
   }

   // attach foot sensor shape
   b2PolygonShape foot_sensor_shape;
   foot_sensor_shape.SetAsBox(
      (width_px / 2.0f) / (PPM * 2.0f), (height_px / 4.0f) / (PPM * 2.0f), b2Vec2(0.0f, (height_px * 0.5f) / (PPM * 2.0f)), 0.0f
   );

   b2FixtureDef foot_sensor_fixture_def;
   foot_sensor_fixture_def.isSensor = true;
   foot_sensor_fixture_def.shape = &foot_sensor_shape;

   _foot_sensor_fixture = _body->CreateFixture(&foot_sensor_fixture_def);
   auto* foot_object_data = new FixtureNode(this);
   foot_object_data->setType(ObjectTypePlayerFootSensor);
   _foot_sensor_fixture->SetUserData(static_cast<void*>(foot_object_data));

   // attach head sensor shape
   b2PolygonShape head_polygon_shape;
   head_polygon_shape.SetAsBox(
      (width_px / 2.0f) / (PPM * 2.0f), (height_px / 4.0f) / (PPM * 2.0f), b2Vec2(0.0f, -height_px / (PPM * 2.0f)), 0.0f
   );

   b2FixtureDef head_sensor_fixture_def;
   head_sensor_fixture_def.isSensor = true;
   head_sensor_fixture_def.shape = &head_polygon_shape;

   auto* head_sensor_fixture = _body->CreateFixture(&head_sensor_fixture_def);
   auto* head_object_data = new FixtureNode(this);
   head_object_data->setType(ObjectTypePlayerHeadSensor);
   head_sensor_fixture->SetUserData(static_cast<void*>(head_object_data));

   // wallslide sensors
   b2PolygonShape left_arm_polygon_shape;
   left_arm_polygon_shape.SetAsBox(
      wall_slide_sensor_width / (PPM * 2.0f),
      wall_slide_sensor_height / (PPM * 2.0f),
      b2Vec2(-wall_slide_sensor_distance, -height_px / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef left_arm_sensor_fixture_def;
   left_arm_sensor_fixture_def.isSensor = true;
   left_arm_sensor_fixture_def.shape = &left_arm_polygon_shape;

   auto* left_arm_sensor_fixture = _body->CreateFixture(&left_arm_sensor_fixture_def);
   auto* left_arm_object_data = new FixtureNode(this);
   left_arm_object_data->setType(ObjectTypePlayerLeftArmSensor);
   left_arm_sensor_fixture->SetUserData(static_cast<void*>(left_arm_object_data));

   b2PolygonShape right_arm_polygon_shape;
   right_arm_polygon_shape.SetAsBox(
      wall_slide_sensor_width / (PPM * 2.0f),
      wall_slide_sensor_height / (PPM * 2.0f),
      b2Vec2(wall_slide_sensor_distance, -height_px / (PPM * 2.0f)),
      0.0f
   );

   b2FixtureDef right_arm_sensor_fixture_def;
   right_arm_sensor_fixture_def.isSensor = true;
   right_arm_sensor_fixture_def.shape = &right_arm_polygon_shape;

   auto* right_arm_sensor_fixture = _body->CreateFixture(&right_arm_sensor_fixture_def);
   auto* right_arm_object_data = new FixtureNode(this);
   right_arm_object_data->setType(ObjectTypePlayerRightArmSensor);
   right_arm_sensor_fixture->SetUserData(static_cast<void*>(right_arm_object_data));
}

void Player::createBody()
{
   // create player body
   auto* body_def = new b2BodyDef();
   body_def->position.Set(getPixelPositionFloat().x * MPP, getPixelPositionFloat().y * MPP);

   body_def->type = b2_dynamicBody;

   _body = _world->CreateBody(body_def);
   _body->SetFixedRotation(true);

   // add body shape
   b2FixtureDef body_fixture_def;
   body_fixture_def.density = 0.45f;
   body_fixture_def.friction = PhysicsConfiguration::getInstance()._player_friction;
   body_fixture_def.restitution = 0.0f;

   body_fixture_def.filter.categoryBits = category_bits;
   body_fixture_def.filter.maskBits = mask_bits_standing;
   body_fixture_def.filter.groupIndex = group_index;

   b2PolygonShape body_shape;
   body_shape.SetAsBox(0.16f, 0.3f, {0.0f, -0.2f}, 0.0f);
   body_fixture_def.shape = &body_shape;

   _body_fixture = _body->CreateFixture(&body_fixture_def);

   auto* object_data_head = new FixtureNode(this);
   object_data_head->setType(ObjectTypePlayer);
   object_data_head->setFlag("head", true);
   _body_fixture->SetUserData(static_cast<void*>(object_data_head));

   // mBody->Dump();

   // store body inside player jump
   _jump._body = _body;
}

void Player::createPlayerBody()
{
   createBody();
   createFeet();
}

void Player::updateFadeOut(const sf::Time& dt)
{
   if (!_fade_out)
   {
      return;
   }

   _fade_out_alpha = std::max(_fade_out_alpha - dt.asSeconds() * _fade_out_speed_factor, 0.0f);
}

void Player::setWorld(const std::shared_ptr<b2World>& world)
{
   _world = world;
}

void Player::resetWorld()
{
   _world.reset();
}

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

float Player::readVelocityFromController(const PlayerSpeed& speed) const
{
   if (_controls->isLookingAround())
   {
      return 0.0f;
   }

   auto axis_value_normalized = _controls->readControllerNormalizedHorizontal();

   // controller is not used, so slow down
   if (fabs(axis_value_normalized) <= 0.3f)
   {
      return speed._current_velocity.x * speed._deceleration;
   }

   axis_value_normalized *= speed._acceleration;

   // checking for the current speed here because even if the player pushes a controller axis
   // to the left side, he might still dash to the other side with quite a strong impulse.
   // that would confuse the speed capping and accelerate to infinity. true story.
   auto desired_velocity = 0.0f;
   if (speed._current_velocity.x < 0.0f)
   {
      desired_velocity = b2Max(speed._current_velocity.x + axis_value_normalized, -speed._velocity_max);
   }
   else
   {
      desired_velocity = b2Min(speed._current_velocity.x + axis_value_normalized, speed._velocity_max);
   }

   // Log::Info()
   //    << "desired: " << desiredVel << " "
   //    << "current: " << speed.currentVelocity.x << " "
   //    << "axis value: " << axisValueNormalized << " "
   //    << "max: " << speed.velocityMax;

   return desired_velocity;
}

bool Player::isPointingRight() const
{
   return !_points_to_left;
}

bool Player::isPointingLeft() const
{
   return _points_to_left;
}

void Player::updateOrientation()
{
   if (isDead())
   {
      return;
   }

   const auto orientation = _controls->updateOrientation();
   if (orientation == PlayerControls::Orientation::Left)
   {
      _points_to_left = true;
   }
   else if (orientation == PlayerControls::Orientation::Right)
   {
      _points_to_left = false;
   }
}

float Player::readVelocityFromKeyboard(const PlayerSpeed& speed) const
{
   if (_controls->isLookingAround())
   {
      return 0.0f;
   }

   const auto attempt_move_left = _controls->isMovingLeft();
   const auto attempt_move_right = _controls->isMovingRight();

   // sanity check to avoid moonwalking
   if (attempt_move_left && attempt_move_right)
   {
      return 0.0f;
   }

   auto desired_velocity = 0.0f;

   if (attempt_move_left)
   {
      desired_velocity = b2Max(speed._current_velocity.x - speed._acceleration, -speed._velocity_max);
   }

   if (attempt_move_right)
   {
      desired_velocity = b2Min(speed._current_velocity.x + speed._acceleration, speed._velocity_max);
   }

   // slowdown as soon as
   // a) no movement to left or right
   // b) movement is opposite to given direction
   // c) no movement at all
   const auto no_movement_to_left_or_right = !attempt_move_left && !attempt_move_right;

   const auto velocity_opposite_to_given_dir =
      (speed._current_velocity.x < -0.01f && attempt_move_right) || (speed._current_velocity.x > 0.01f && attempt_move_left);

   const auto no_movement = (fabs(desired_velocity) < 0.0001f);

   if (no_movement_to_left_or_right || velocity_opposite_to_given_dir || no_movement)
   {
      desired_velocity = speed._current_velocity.x * speed._deceleration;
   }

   return desired_velocity;
}

float Player::getDeceleration() const
{
   auto deceleration = (isInAir()) ? PhysicsConfiguration::getInstance()._player_deceleration_air
                                   : PhysicsConfiguration::getInstance()._player_deceleration_ground;

   return deceleration;
}

float Player::getAcceleration() const
{
   auto acceleration = (isInAir()) ? PhysicsConfiguration::getInstance()._player_acceleration_air
                                   : PhysicsConfiguration::getInstance()._player_acceleration_ground;

   return acceleration;
}

bool Player::isDead() const
{
   return _dead;
}

bool Player::isJumpingThroughOneWayWall()
{
   // a player is considered jumping through a one-way wall when
   // - the y velocity goes up
   // - there are active contacts with a one-way wall
   constexpr auto epsilon = 0.00001f;
   if (_body->GetLinearVelocity().y < -epsilon)
   {
      if (OneWayWall::instance().hasContacts())
      {
         return true;
      }
   }

   return false;
}

void Player::updateAnimation(const sf::Time& dt)
{
   PlayerAnimation::PlayerAnimationData data;

   data._dead = isDead();
   data._death_count_current_level = SaveState::getPlayerInfo()._stats._death_count_current_level;
   data._checkpoint_index = SaveState::getCurrent()._checkpoint;
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
   data._dash_frame_count = _dash._frame_count;
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
   data._timepoint_attack_start = _attack._timepoint_attack_start;
   data._timepoint_attack_standing_start = _attack._timepoint_attack_standing_start;
   data._timepoint_attack_bend_down_start = _attack._timepoint_attack_bend_down_start;
   data._timepoint_attack_jumping_start = _attack._timepoint_attack_jumping_start;
   data._attacking = _attack.isAttacking();

   const auto& weapon_system = SaveState::getPlayerInfo()._weapons;
   data._weapon_type = (!weapon_system._selected) ? WeaponType::None : weapon_system._selected->getWeaponType();

   if (_dash.hasMoreFrames())
   {
      data._dash_dir = _dash._direction;
   }

   // pick latest left/right input to avoid conflicts
   if (data._moving_right && data._moving_left)
   {
      if (_controls->wasMovingLeft())
      {
         data._moving_left = false;
      }

      if (_controls->wasMovingRight())
      {
         data._moving_right = false;
      }
   }

   _player_animation->update(dt, data);
}

float Player::readDesiredVelocity() const
{
   const auto acceleration = getAcceleration();
   const auto deceleration = getDeceleration();
   const auto current_velocity = _body->GetLinearVelocity();
   const auto velocity_max = getMaxVelocity();

   PlayerSpeed speed{current_velocity, velocity_max, acceleration, deceleration};

   const auto desired_velocity = readDesiredVelocity(speed);
   return desired_velocity;
}

float Player::readDesiredVelocity(const PlayerSpeed& speed) const
{
   auto desired_velocity = 0.0f;

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      if (_controls->isControllerUsedLast())
      {
         desired_velocity = readVelocityFromController(speed);
      }
      else
      {
         desired_velocity = readVelocityFromKeyboard(speed);
      }
   }
   else
   {
      desired_velocity = readVelocityFromKeyboard(speed);
   }

   return desired_velocity;
}

void Player::updateVelocity()
{
   using namespace std::chrono_literals;

   if (isDead())
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (Portal::isLocked())
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (_hard_landing)
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   // block movement while spawning
   // spawn is only played if player has died before in current level
   if (GameClock::getInstance().durationSinceSpawn() < _player_animation->getRevealDuration() &&
       SaveState::getPlayerInfo()._stats._death_count_current_level > 0)
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (StopWatch::getInstance().now() < _attack._timepoint_attack_standing_start + _player_animation->getSwordAttackDurationStanding())
   {
      _body->SetLinearVelocity({0.0, 0.0});
      return;
   }

   if (ScreenTransitionHandler::getInstance().active())
   {
      const auto velocity = _body->GetLinearVelocity();
      _body->SetLinearVelocity({0.0, velocity.y});
      return;
   }

   if (_bend._bending_down)
   {
      if (!(SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(Skill::SkillType::Crouch)))
      {
         if (getControls()->isDroppingDown() && OneWayWall::instance().hasContacts())
         {
            // usually just stop the player from movement when bending down while he has no crouching ability
            // however, when dropping from a platform, we don't want to mess with the velocity,
            // just let the player fall
         }
         else
         {
            // while the player is standing on a platform, he is allowed to be to bend down
            // however, he is not capable of changing his velocity by pressing left or right
            const auto velocity = _body->GetLinearVelocity();
            _body->SetLinearVelocity({_belt.isOnBelt() ? _belt.getBeltVelocity() : 0.0f, velocity.y});
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

   auto desired_velocity = readDesiredVelocity();
   auto current_velocity = _body->GetLinearVelocity();

   // physically so wrong but gameplay-wise the best choice :)
   _belt.applyBeltVelocity(desired_velocity, getMaxVelocity(), _controls);

   // calc impulse, disregard time factor
   auto velocity_change_x = desired_velocity - current_velocity.x;
   auto impulse_x = _body->GetMass() * velocity_change_x;

   _body->ApplyLinearImpulse(b2Vec2(impulse_x, 0.0f), _body->GetWorldCenter(), true);

   // simulate some friction when moving underwater, also apply some buoyancy
   if (isInWater())
   {
      const auto buoyancy_force = PhysicsConfiguration::getInstance()._in_water_buoyancy_force * -_world->GetGravity();
      _body->ApplyForce(buoyancy_force, _body->GetWorldCenter(), true);

      auto linear_velocity = _body->GetLinearVelocity();

      linear_velocity.Set(
         linear_velocity.x,
         std::clamp(
            linear_velocity.y,
            PhysicsConfiguration::getInstance()._player_in_water_linear_velocity_y_clamp_min,
            PhysicsConfiguration::getInstance()._player_in_water_linear_velocity_y_clamp_max
         )
      );

      _body->SetLinearVelocity(linear_velocity);
   }

   // cap speed
   const auto& physics_config = PhysicsConfiguration::getInstance();
   const auto max_velocity_horizontal = physics_config._player_max_velocity_horizontal;
   const auto max_velocity_up = physics_config._player_max_velocity_up;
   const auto max_velocity_down = physics_config._player_max_velocity_down;
   const auto linear_velocity = _body->GetLinearVelocity();

   _body->SetLinearVelocity(
      {std::clamp(linear_velocity.x, -max_velocity_horizontal, max_velocity_horizontal),
       std::clamp(linear_velocity.y, -max_velocity_up, max_velocity_down)}
   );
}

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

   return screen_transition;
}

const Chunk& Player::getChunk() const
{
   return _chunk;
}

const PlayerBend& Player::getBend() const
{
   return _bend;
}

const PlayerJump& Player::getJump() const
{
   return _jump;
}

PlayerBelt& Player::getBelt()
{
   return _belt;
}

PlayerPlatform& Player::getPlatform()
{
   return _platform;
}

void Player::goToPortal(auto portal)
{
   auto dst_position_px = portal->getDestination()->getPortalPosition();

   setBodyViaPixelPosition(dst_position_px.x + PLAYER_ACTUAL_WIDTH / 2, dst_position_px.y + DIFF_PLAYER_TILE_TO_PHYSICS);

   // update the camera system to point to the player position immediately
   CameraSystem::getInstance().syncNow();
   Portal::unlock();
}

void Player::updatePortal()
{
   if (CameraPanorama::getInstance().isLookActive())
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

      if (_controls->hasFlag(KeyPressedUp) || joystick_points_up)
      {
         auto portal = PortalWrapper::getNearbyPortal();
         if (portal && portal->getDestination())
         {
            Portal::lock();
            _portal_clock.restart();

            auto screen_transition = makeFadeTransition();
            screen_transition->_callbacks_effect_1_ended.emplace_back([this, portal]() { goToPortal(portal); });
            screen_transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });
            ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
         }
      }
   }
}

void Player::impulse(float intensity)
{
   // just store the information we get from the post solve call for now.
   // other evaluation from BeginContact / EndContact might make the impulse irrelevant
   _impulse = intensity;
}

void Player::startHardLanding()
{
   _controls->lockOrientation(std::chrono::milliseconds(1000));
   Level::getCurrentLevel()->getBoomEffect().boom(0.0f, 1.0f, BoomSettings{0.5, 0.5f});

   _timepoint_hard_landing = StopWatch::getInstance().now();
   _hard_landing = true;
   _hard_landing_cycles = 0;

   auto& gji = GameControllerIntegration::getInstance();
   if (gji.isControllerConnected())
   {
      if (GameConfiguration::getInstance()._rumble_enabled)
      {
         gji.getController()->rumble(0.5f, 300);
      }
   }

   Audio::getInstance().playSample({"player_grunt_01.wav"});
}

void Player::updateImpulse()
{
   if (_impulse < impulse_epsilon)
   {
      return;
   }

   auto impulse_value = _impulse;
   _impulse = 0.0f;

   if (GameContactListener::getInstance().isPlayerSmashed())
   {
      return;
   }

   // const auto dx = _velocity_previous.x - _body->GetLinearVelocity().x;
   // const auto dy = _velocity_previous.y - _body->GetLinearVelocity().y;
   //
   // Log::Info()
   //    << "intensity: " << intensity
   //    << " dx: " << dx
   //    << " dy: " << dy
   //    << " dir: " << (horizontal ? "x" : "y");
   //
   // const auto horizontal = (fabs(dx) > fabs(dy));
   // if (horizontal)
   // {
   //    if (impulse > 0.4f)
   //    {
   //       Level::getCurrentLevel()->getBoomEffect().boom(0.2f, 0.0f);
   //    }
   // }

   if (impulse_value > 1.0f)
   {
      if (BouncerWrapper::getNearbyBouncer())
      {
         return;
      }

      startHardLanding();

      if (PhysicsConfiguration::getInstance()._player_hard_landing_damage_enabled)
      {
         damage(static_cast<int32_t>((impulse_value - 1.0f) * PhysicsConfiguration::getInstance()._player_hard_landing_damage_factor));
      }
   }
}

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

   if (SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(Skill::SkillType::Invulnerable))
   {
      return;
   }

   if (_damage_clock.getElapsedTime().asMilliseconds() > 3000)
   {
      _damage_initialized = true;

      Audio::getInstance().playSample({"hurt.wav"});

      // not converting this to PPM to make the effect of the applied force more visible
      auto* body = getBody();
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

void Player::kill(std::optional<DeathReason> death_reason)
{
   if (SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(Skill::SkillType::Invulnerable))
   {
      return;
   }

   damage(1000);

   if (death_reason.has_value())
   {
      _death_reason = death_reason;
   }
}

bool Player::isOnGround() const
{
   return GameContactListener::getInstance().getPlayerFootContactCount() > 0;
}

void Player::updateAttack()
{
   // at the moment the game doesn't have any in-water attacks
   if (isInWater())
   {
      return;
   }

   const auto& inventory = SaveState::getPlayerInfo()._inventory;

   const auto x_button_pressed = _controls->isButtonXPressed();
   const auto y_button_pressed = _controls->isButtonYPressed();
   _attack._attack_button_was_pressed = _attack._attack_button_pressed;
   _attack._attack_button_pressed = InventoryBasedControls::isAttackButtonPressed(inventory, x_button_pressed, y_button_pressed);

   // there are weapons that support continous attacks while the button is down, others like the sword
   // require a fresh button press each time the sword should be swung
   if (_attack._attack_button_pressed)
   {
      _attack.attack(_world, _controls, _player_animation, _pixel_position_f, _points_to_left, isInAir());
   }
}

bool Player::isInWater() const
{
   return _in_water;
}

void Player::setInWater(bool in_water)
{
   _in_water = in_water;
}

void Player::updateFootsteps()
{
   if (GameContactListener::getInstance().getPlayerFootContactCount() > 0 && !isInWater())
   {
      auto vel = fabs(_body->GetLinearVelocity().x);
      if (vel > 0.1f)
      {
         if (vel < 3.0f)
         {
            vel = 3.0f;
         }

         if (_time.asSeconds() > _next_footstep_time)
         {
            // play footstep
            Audio::getInstance().playSample({(_step_counter++ & 1) ? "player_footstep_stone_l.wav" : "player_footstep_stone_r.wav", 0.3f});
            _next_footstep_time = _time.asSeconds() + 1.0f / vel;
         }
      }
   }
}

int Player::getId() const
{
   return _id;
}

int Player::getZIndex() const
{
   return _z_index;
}

void Player::setZIndex(int32_t z)
{
   _z_index = z;
}

void Player::updateBendDown()
{
   // disable bend down states when player hit dash button
   if (_dash.hasMoreFrames())
   {
      _bend._was_bending_down = false;
      _bend._bending_down = false;
      return;
   }

   auto down_pressed = _controls->isBendDownActive();

   // if the head touches something while crouches, keep crouching
   if (_bend._bending_down && !down_pressed && (GameContactListener::getInstance().getPlayerHeadContactCollidingCount() > 0))
   {
      return;
   }

   if (!_bend._bending_down && CameraPanorama::getInstance().isLookActive())
   {
      return;
   }

   const auto bending_down = down_pressed && !isInAir() && !isInWater();

   _bend._was_bending_down = _bend._bending_down;
   _bend._bending_down = bending_down;

   if (!_bend._was_bending_down && _bend._bending_down)
   {
      _bend._timepoint_bend_down_start = StopWatch::getInstance().now();
      Audio::getInstance().playSample({"player_kneel_01.wav"});
   }

   // when the player transitions from "was bending down" to "no longer bending down", we want to
   // store the timepoint that's used for the "bending up" animation. however, that is actually only
   // relevant when the player is not in the air right now. that is because the player can also hit
   // down + jump on one way walls to drop through. in that case we don't want any bend up frames
   // to mess up any of the subsequent animation frames.
   if (_bend._was_bending_down && !_bend._bending_down && !isInAir())
   {
      _bend._timepoint_bend_down_end = StopWatch::getInstance().now();
   }

   setMaskBitsCrouching(bending_down);
}

void Player::updateHardLanding()
{
   using namespace std::chrono_literals;

   if (_hard_landing)
   {
      _hard_landing_cycles++;

      const auto hard_landing_time_elapsed_s =
         std::chrono::duration<double>(StopWatch::getInstance().now() - _timepoint_hard_landing).count();

      if (hard_landing_time_elapsed_s > PhysicsConfiguration::getInstance()._player_hard_landing_delay_s)
      {
         _hard_landing = false;
         _hard_landing_cycles = 0;
      }
   }

   if (_hard_landing_cycles > 1)
   {
      // if player does a hard landing on a moving platform, we don't want to reset the linear velocity.
      // maybe come up with a nice concept for this one day.
      if (_platform.isOnPlatform())
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

   float closest_fraction = 1.0f;
   b2Vec2 intersection_normal(0.0f, -1.0f);

   if (!_ground_body)
   {
      _ground_normal.Set(0.0f, -1.0f);
      return;
   }

   for (auto* f = _ground_body->GetFixtureList(); f; f = f->GetNext())
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
         {
            continue;
         }

         if (output.fraction < closest_fraction)
         {
            closest_fraction = output.fraction;
            intersection_normal = output.normal;
         }
      }
   }

   _ground_normal = intersection_normal;
}

void Player::updateOneWayWallDrop()
{
   if (getControls()->isDroppingDown())
   {
      OneWayWall::instance().drop();
   }
}

void Player::updateChainShapeCollisions()
{
   if (_jump.isJumping())
   {
      return;
   }

   const auto hiccup_pos = ChainShapeAnalyzer::checkPlayerAtCollisionPosition();
   if (!hiccup_pos.has_value())
   {
      return;
   }

   const auto velocity = _body->GetLinearVelocity();
   const auto pos = _body->GetPosition();
   _body->SetLinearVelocity({velocity.x, 0.0f});
   _body->SetTransform({pos.x, hiccup_pos->y - 0.18f}, 0.0f);
}

void Player::updateJump()
{
   PlayerJump::PlayerJumpInfo info;

   info._in_air = isInAir();
   info._in_water = isInWater();
   info._water_entered_timepoint = _water_entered_time;
   info._crouching = _bend.isCrouching();
   info._climbing = _climb.isClimbing();
   info._dashing = _dash.hasMoreFrames();

   _jump.update(info);
}

void Player::updateWallslide(const sf::Time& dt)
{
   const auto wallslide_animation = _player_animation->getWallslideAnimation();
   const auto offset_x_px = isPointingLeft() ? -5.0f : 5.0f;
   wallslide_animation->setPosition(_pixel_position_f.x + offset_x_px, _pixel_position_f.y);
   wallslide_animation->play();
   wallslide_animation->update(dt);
}

void Player::updateWaterBubbles(const sf::Time& dt)
{
   WaterBubbles::WaterBubbleInput input;
   input._player_in_water = isInWater();
   input._player_rect = getPixelRectFloat();
   input._player_pointing_right = isPointingRight();
   _water_bubbles.update(dt, input);
}

void Player::updateSpawn()
{
   using namespace std::chrono_literals;

   if (GameClock::getInstance().durationSinceSpawn() < 1.0s)
   {
      if (!_spawn_orientation_locked)
      {
         _spawn_orientation_locked = true;
         const auto lock_duration = std::chrono::duration_cast<std::chrono::milliseconds>(_player_animation->getRevealDuration());

         // appear animation is shown after spawn is complete
         // for that duration of time, lock the player orientation
         _controls->lockOrientation(lock_duration);
      }

      return;
   }

   if (_spawn_complete)
   {
      return;
   }

   _spawn_complete = true;

   // play reveal sound (but only if player died earlier)
   if (SaveState::getPlayerInfo()._stats._death_count_current_level > 0)
   {
      Audio::getInstance().playSample({"player_spawn_01.wav"});
   }
}

void Player::updateHealth(const sf::Time& dt)
{
   SaveState::getPlayerInfo()._extra_table._health.update(dt);
}

void Player::update(const sf::Time& dt)
{
   _time += dt;

   // a lot depends on an up-to-date pixel position and the hitbox that's generated out of it
   updatePixelPosition();
   updatePixelRect();
   updateChunk();
   _animation_pool.updateAnimations(dt);
   updateFadeOut(dt);
   updateHealth(dt);
   updateChainShapeCollisions();
   updateImpulse();
   updateGroundAngle();
   updateHardLanding();
   updateBendDown();
   updateAnimation(dt);
   updatePixelCollisions();
   updateAtmosphere();
   updateAttack();
   updateVelocity();
   updateOrientation();
   updateOneWayWallDrop();
   updateJump();
   updateDash();
   _climb.update(_body, isInAir());
   _platform.update(_body, _jump.isJumping());
   PlayerAudio::updateListenerPosition(_pixel_position_f);
   updateFootsteps();
   updatePortal();
   updatePreviousBodyState();
   updateWeapons(dt);
   updateWallslide(dt);
   updateWaterBubbles(dt);
   updateSpawn();

   _controls->update(dt);  // called at last just to backup previous controls
}

void Player::reloadAnimationPool()
{
   _animation_pool.reload();
   _player_animation->loadAnimations(_animation_pool);
}

void Player::resetMotionBlur()
{
   _last_animations.clear();
   _player_animation->resetAlpha();
}

void Player::updateDash(Dash dir)
{
   PlayerDash::DashInput input{dir, _jump._wallsliding, _hard_landing, isInWater(), _points_to_left, _body};
   _dash.update(input);
}

void Player::updatePixelCollisions()
{
   const auto& rect = getPixelRectFloat();
   Laser::collide(rect);
   Fan::collide(rect, _body);
}

void Player::updateAtmosphere()
{
   const auto was_inside_water = isInWater();

   const auto& pos = _body->GetPosition();
   auto tile = Level::getCurrentLevel()->getAtmosphere().getTileForPosition(pos);

   const auto inside_water = (tile >= AtmosphereTileWaterFull && tile <= AtmosphereTileWaterCornerTopLeft);
   setInWater(inside_water);

   // entering water
   if (inside_water && !was_inside_water)
   {
      _body->SetGravityScale(PhysicsConfiguration::getInstance()._gravity_scale_water);
      _body->SetTransform(_body->GetPosition() + b2Vec2{0.0, 0.4f}, 0.0f);
      _water_entered_time = StopWatch::getInstance().now();
      Audio::getInstance().playSample({"splash.wav"});
      _animation_pool.create("player_water_splash", _pixel_position_f.x, _pixel_position_f.y);
   }

   // leaving water
   if (!inside_water && was_inside_water)
   {
      _body->SetGravityScale(PhysicsConfiguration::getInstance()._gravity_scale_default);
      _animation_pool.create("player_water_splash", _pixel_position_f.x, _pixel_position_f.y);
   }

   // not sure if this is just another ugly hack
   // when we leave the water we want to take out the current swimming velocity
   if (was_inside_water && !isInWater())
   {
      _body->SetLinearVelocity(b2Vec2(0, 0));
   }
}

void Player::setGroundBody(b2Body* body)
{
   _ground_body = body;
}

bool Player::getVisible() const
{
   return _visible;
}

void Player::setVisible(bool visible)
{
   _visible = visible;
}

void Player::fadeOut(float fade_out_speed_factor)
{
   _fade_out = true;
   _fade_out_speed_factor = fade_out_speed_factor;
}

void Player::fadeOutReset()
{
   _fade_out = false;
   _fade_out_alpha = 1.0f;
}

void Player::setFriction(float friction)
{
   for (auto* fixture = _body->GetFixtureList(); fixture; fixture = fixture->GetNext())
   {
      fixture->SetFriction(friction);
   }

   for (auto* contact = _body->GetContactList(); contact; contact = contact->next)
   {
      contact->contact->ResetFriction();
   }
}

bool Player::isInAir() const
{
   return (GameContactListener::getInstance().getPlayerFootContactCount() == 0) && !isInWater();
}

void Player::updateWeapons(const sf::Time& dt)
{
   for (auto& w : SaveState::getPlayerInfo()._weapons._weapons)
   {
      w->update(dt);
   }
}

void Player::die()
{
   _dead = true;
   Audio::getInstance().playSample({"death.wav"});
   SaveState::getPlayerInfo()._stats._death_count_overall++;
   SaveState::getPlayerInfo()._stats._death_count_current_level++;
}

void Player::reset()
{
   // check for checkpoints
   // so start position could vary here
   _hard_landing = false;
   _hard_landing_cycles = 0;

   if (_body)
   {
      _body->SetLinearVelocity(b2Vec2(0, 0));
      _body->SetGravityScale(1.0);
   }

   _climb.removeClimbJoint();

   if (Level::getCurrentLevel())
   {
      setBodyViaPixelPosition(Level::getCurrentLevel()->getStartPosition().x, Level::getCurrentLevel()->getStartPosition().y);
   }

   SaveState::getPlayerInfo()._extra_table._health.reset();

   // resetting any player info apart from the health doesn't make sense since it's loaded from disk when the player dies
   // SaveState::getPlayerInfo().mInventory.resetKeys();

   // reset bodies passed from the contact listener
   _ground_body = nullptr;

   // reset dash
   _dash._frame_count = 0;
   resetMotionBlur();
   _dead = false;
   _death_reason.reset();
   _spawn_complete = false;
   _spawn_orientation_locked = false;
   _in_water = false;

   fadeOutReset();
}

DeathReason Player::checkDead() const
{
   DeathReason reason = DeathReason::Invalid;

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

   // death reason can also be set externally, and then should overrule the internal detection
   if (_death_reason.has_value())
   {
      reason = _death_reason.value();
   }

   return reason;
}

void Player::setStartPixelPosition(float x, float y)
{
   setPixelPosition(x, y);
}

void Player::traceJumpCurve()
{
   if (_controls->isButtonAPressed())
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
         std::cout << _time.asSeconds() - _jump_trace._jump_start_time.asSeconds() << "; " << jumpNextY << std::endl;
      }

      _jump_trace._jump_prev_y = jumpNextY;
   }
   else
   {
      _jump_trace._jump_started = false;
   }
}

void Player::keyPressed(sf::Keyboard::Key key)
{
   switch (key)
   {
      case sf::Keyboard::Space:
      {
         _jump.jump();
         break;
      }
      case sf::Keyboard::Z:
      {
         updateDash(Dash::Left);
         break;
      }
      case sf::Keyboard::X:
      {
         updateDash(Dash::Right);
         break;
      }
      case sf::Keyboard::LControl:
      {
         useInventory(0);
         break;
      }
      case sf::Keyboard::LAlt:
      {
         useInventory(1);
         break;
      }
      default:
      {
         break;
      }
   }
}

const std::shared_ptr<PlayerControls>& Player::getControls() const
{
   return _controls;
}

void Player::updatePixelPosition()
{
   if (isDead())
   {
      return;
   }

   // sync player sprite with with box2d data
   const auto x = _body->GetPosition().x * PPM;
   const auto y = _body->GetPosition().y * PPM;

   setPixelPosition(x, y);
}

void Player::updatePreviousBodyState()
{
   _position_previous = _body->GetPosition();
   _velocity_previous = _body->GetLinearVelocity();
}
