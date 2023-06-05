
#include "bubblecube.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/globalclock.h"
#include "level.h"
#include "player/player.h"
#include "texturepool.h"

#include <iostream>

// #define DEBUG_COLLISION_RECTS 1

#ifdef DEBUG_COLLISION_RECTS
#include "debugdraw.h"
#endif

namespace
{
static constexpr auto width_px = 36;
static constexpr auto height_px = 36;
static constexpr auto bevel_px = 6;
static constexpr auto width_m = width_px * MPP;
static constexpr auto height_m = height_px * MPP;
static constexpr auto bevel_m = bevel_px * MPP;

static constexpr auto columns = 12;
static constexpr auto tiles_per_box_width = 4;
static constexpr auto tiles_per_box_height = 3;

static constexpr auto move_amplitude = 0.08f;
static constexpr auto move_frequency = 4.19f;

static constexpr auto pop_frequency = 15.0f;

static constexpr auto sprite_offset_x_px = -(width_px - bevel_px);
static constexpr auto sprite_offset_y_px = -14;

static constexpr auto collision_rect_height = 10;
}  // namespace

BubbleCube::BubbleCube(GameNode* parent, const GameDeserializeData& data) : FixtureNode(parent)
{
   setClassName(typeid(BubbleCube).name());
   setType(ObjectTypeBubbleCube);
   setObjectId(data._tmx_object->_name);

   // read properties
   if (data._tmx_object->_properties)
   {
      const auto animation_offset_it = data._tmx_object->_properties->_map.find("animation_offset_s");
      if (animation_offset_it != data._tmx_object->_properties->_map.end())
      {
         _animation_offset_s = animation_offset_it->second->_value_float.value();
      }

      const auto pop_time_respawn_it = data._tmx_object->_properties->_map.find("pop_time_respawn_s");
      if (pop_time_respawn_it != data._tmx_object->_properties->_map.end())
      {
         _pop_time_respawn_s = pop_time_respawn_it->second->_value_float.value();
      }

      const auto move_down_on_contact_it = data._tmx_object->_properties->_map.find("move_down_on_contact");
      if (move_down_on_contact_it != data._tmx_object->_properties->_map.end())
      {
         _move_down_on_contact = move_down_on_contact_it->second->_value_bool.value();
      }

      const auto move_down_velocity_it = data._tmx_object->_properties->_map.find("move_down_velocity");
      if (move_down_velocity_it != data._tmx_object->_properties->_map.end())
      {
         _move_down_velocity = move_down_velocity_it->second->_value_float.value();
      }

      const auto maximum_contact_duration_s_it = data._tmx_object->_properties->_map.find("maximum_contact_duration_s");
      if (maximum_contact_duration_s_it != data._tmx_object->_properties->_map.end())
      {
         _maximum_contact_duration_s = maximum_contact_duration_s_it->second->_value_float.value();
      }

      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         setZ(static_cast<uint32_t>(z_it->second->_value_int.value()));
      }
   }

   // set up shape
   //
   //       0        7
   //       +--------+
   //      /          \
   //   1 +            + 6
   //     |            |
   //   2 +            + 5
   //      \          /
   //       +--------+
   //       3        4

   std::array<b2Vec2, 8> vertices{
      b2Vec2{bevel_m, 0.0f},
      b2Vec2{0.0f, bevel_m},
      b2Vec2{0.0f, height_m - bevel_m},
      b2Vec2{bevel_m, height_m},
      b2Vec2{width_m - bevel_m, height_m},
      b2Vec2{width_m, height_m - bevel_m},
      b2Vec2{width_m, bevel_m},
      b2Vec2{width_m - bevel_m, 0.0f},
   };

   _shape.Set(vertices.data(), static_cast<int32_t>(vertices.size()));

   // create body
   _x_px = data._tmx_object->_x_px;
   _y_px = data._tmx_object->_y_px;
   _position_m = MPP* b2Vec2{_x_px - width_px / 2, _y_px};

   b2BodyDef body_def;
   body_def.type = b2_dynamicBody;
   body_def.position = _position_m;
   _body = data._world->CreateBody(&body_def);

   // set up body fixture
   b2FixtureDef fixture_def;
   fixture_def.shape = &_shape;
   fixture_def.density = 1.0f;
   fixture_def.isSensor = false;
   _fixture = _body->CreateFixture(&fixture_def);
   _fixture->SetUserData(static_cast<void*>(this));

   // pin the cube to its anchor
   _anchor_def.position = b2Vec2(_position_m.x + width_m * 0.5f, _position_m.y - 0.1f);
   _anchor_body = data._world->CreateBody(&_anchor_def);
   auto anchor_fixture = _anchor_body->CreateFixture(&_anchor_a_shape, 0.0f);
   anchor_fixture->SetSensor(true);

   // Step 3: Define and create the prismatic joint
   b2PrismaticJointDef prismaticJointDef;
   prismaticJointDef.bodyA = _anchor_body;
   prismaticJointDef.bodyB = _body;
   prismaticJointDef.collideConnected = false;

   // Step 4: Set the local axis along the y-axis
   prismaticJointDef.localAxisA.Set(0.0f, 1.0f);

   // Step 5: Adjust other parameters of the prismatic joint
   prismaticJointDef.enableLimit = true;
   prismaticJointDef.lowerTranslation = 0.0f;  // Minimum allowed position along the y-axis
   prismaticJointDef.upperTranslation = 100.0f;  // Maximum allowed position along the y-axis
   prismaticJointDef.enableMotor = true;
   prismaticJointDef.motorSpeed = 0.0f;      // Speed at which the body moves along the y-axis
   prismaticJointDef.maxMotorForce = 10.0f;  // Maximum force applied by the motor

   _joint = (b2PrismaticJoint*)data._world->CreateJoint(&prismaticJointDef);

   // set up visualization
   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "bubble_cube.png");
   _sprite.setTexture(*_texture);

   _fixed_rect_px = {data._tmx_object->_x_px, data._tmx_object->_y_px, width_px, height_px};
}

// 12 x 4 boxes per row
//
// regular animation is in row 0
// pop animation is in row 1
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+ . . .
// |   | __|__ |   |   | __|__ |   |   | __|__ |   |   | __|__ |   |
// +---+/#####\+---+---+/#####\+---+---+/#####\+---+---+/#####\+---+ . . .
// |   |#######|   |   |#######|   |   |#######|   |   |#######|   |
// +---+\#####/+---+---+\#####/+---+---+\#####/+---+---+\#####/+---+ . . .
// |   | ""|"" |   |   | ""|"" |   |   | ""|"" |   |   | ""|"" |   |
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+ . . .
void BubbleCube::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   auto sprite_index = 0;

   if (_popped)
   {
      sprite_index = std::min(static_cast<int32_t>(_pop_elapsed_s * pop_frequency), columns - 1);
   }
   else
   {
      sprite_index = static_cast<int32_t>(_mapped_value_normalized * columns + 6) % columns;
   }

   _sprite.setTextureRect(
      {sprite_index * PIXELS_PER_TILE * tiles_per_box_width,
       (_popped ? 1 : 0) * PIXELS_PER_TILE * tiles_per_box_height,
       PIXELS_PER_TILE * tiles_per_box_width,
       PIXELS_PER_TILE * tiles_per_box_height}
   );

   color.draw(_sprite);

#ifdef DEBUG_COLLISION_RECTS
   auto fixed_rect_moved_down_px = _fixed_rect_px;
   fixed_rect_moved_down_px.top += static_cast<int32_t>(_push_down_offset_px);
   auto moved_box_rect = _fixed_rect_px;
   moved_box_rect.top = moved_box_rect.top - 12;
   moved_box_rect.left -= 8;
   moved_box_rect.width += 8 * 2;
   DebugDraw::drawRect(color, _foot_collision_rect_px, sf::Color::Magenta);
   DebugDraw::drawRect(color, _fixed_rect_px, sf::Color::Green);
   DebugDraw::drawRect(color, fixed_rect_moved_down_px, sf::Color::Green);
   DebugDraw::drawRect(color, moved_box_rect, sf::Color::Blue);
   DebugDraw::drawRect(color, Player::getCurrent()->computeFootSensorPixelIntRect(), sf::Color::Cyan);
   DebugDraw::drawPoint(color, Player::getCurrent()->getPixelPositionFloat() + sf::Vector2f(0.0f, 10.0f), b2Color(1.0f, 0.5f, 0.5f));
#endif
}

void BubbleCube::updateMaxDurationCondition(const sf::Time& dt)
{
   if (_popped)
   {
      return;
   }

   if (_foot_sensor_triggered_counter == 0)
   {
      return;
   }

   // pop if player exceeds maximum contact duration
   if (_maximum_contact_duration_s.has_value())
   {
      _contact_duration_s += dt.asSeconds();

      if (_contact_duration_s > _maximum_contact_duration_s)
      {
         _contact_duration_s = 0.0f;
         _exceeded_max_contact_duration = true;
      }
   }
}

void BubbleCube::updatePosition()
{
   const auto mapped_value = fmod((_animation_offset_s + _elapsed_s) * move_frequency, static_cast<float>(M_PI) * 2.0f);
   _mapped_value_normalized = mapped_value / (static_cast<float>(M_PI) * 2.0f);

   const auto pos_px = PPM * _body->GetPosition();
   _sprite.setPosition(pos_px.x + sprite_offset_x_px, pos_px.y + sprite_offset_y_px);
}

void BubbleCube::updateRespawnCondition()
{
   // respawn when the time has come
   if (_popped && (GlobalClock::getInstance().getElapsedTime() - _pop_time).asSeconds() > _pop_time_respawn_s)
   {
      // don't respawn while player blocks the area
      if (!Player::getCurrent()->getPixelRectFloat().intersects(_fixed_rect_px))
      {
         _popped = false;
         _body->SetEnabled(true);
      }
   }
}

void BubbleCube::updateFootSensorContact()
{
   if (_popped)
   {
      return;
   }

   //     +-------------+
   //    /|             |\
   //   / |             | \
   //  /  |             |  \
   // /   |             |   \
   // +---+             +---+
   // bevel             bevel
   // +---------------------+
   //          width

   // disabled bevel
   constexpr auto bevel_range_increase_px = 2;
   _foot_collision_rect_px = {
      _x_px + bevel_px - bevel_range_increase_px,
      _y_px + _push_down_offset_px,
      width_px - (2 * bevel_px) + (2 * bevel_range_increase_px),
      collision_rect_height};

   const auto foot_sensor_rect = Player::getCurrent()->computeFootSensorPixelFloatRect();
   _foot_sensor_rect_intersects = foot_sensor_rect.intersects(_foot_collision_rect_px);

   const auto player_moving_down = Player::getCurrent()->getBody()->GetLinearVelocity().y > 0.01f;

   if (_foot_sensor_rect_intersects && player_moving_down)
   {
      _foot_sensor_triggered_counter++;
   }

#ifdef DEBUG_COLLISION_RECTS
   _sprite.setColor(sf::Color(255, _foot_sensor_triggered_counter ? 0 : 255, _foot_sensor_triggered_counter ? 0 : 255));
#endif
}

void BubbleCube::updateJumpedOffPlatformCondition()
{
   auto moved_box_rect = _fixed_rect_px;
   moved_box_rect.top = moved_box_rect.top - 12;
   moved_box_rect.left -= 8;
   moved_box_rect.width += 8 * 2;

   const auto first_jump_frame = (Player::getCurrent()->getJump()._jump_frame_count == 9);
   const auto intersects_box = moved_box_rect.intersects(Player::getCurrent()->computeFootSensorPixelFloatRect());

   if (first_jump_frame && intersects_box)
   {
      _jumped_off_this_platform = true;
   }
}

void BubbleCube::updateMotorSpeed(const sf::Time& dt)
{
   // TOOD
   //
   // when player just landed, show rotation
   //
   // when player is still on bubble, move down
   //
   // when player is off the bubble retract, even if it has been popped

   if (_foot_sensor_triggered_counter > 0)
   {
      _motor_time_s += dt.asSeconds() * 10.0f;
      _motor_speed = sin(_motor_time_s);
      _motor_time_s = std::min(_motor_time_s, static_cast<float>(2.0f * M_PI));
   }
   else
   {
      _motor_speed *= 0.99f;
   }

   _joint->SetMotorSpeed(_motor_speed);
}

void BubbleCube::updatePoppedCondition()
{
   if (_popped)
   {
      return;
   }

   if (_jumped_off_this_platform || _lost_foot_contact || _exceeded_max_contact_duration || _collided_with_surrounding_areas)
   {
      pop();
   }
}

struct BubbleQueryCallback : public b2QueryCallback
{
   std::vector<b2Body*> _bodies;
   b2Body* _body = nullptr;
   std::optional<size_t> _max_count;

   bool checkBelongsToPlayer(b2Fixture* fixture) const
   {
      auto fixture_node = static_cast<FixtureNode*>(fixture->GetUserData().pointer);
      if (!fixture_node)
      {
         return false;
      }

      return dynamic_cast<Player*>(fixture_node->getParent());
   }

   bool ReportFixture(b2Fixture* fixture)
   {
      // filter out player fixtures and the bubble body
      if (checkBelongsToPlayer(fixture))
      {
         return true;
      }
      else if (fixture->GetBody() == _body)
      {
         return true;
      }

      _bodies.push_back(fixture->GetBody());

      if (_max_count.has_value() && _bodies.size() > _max_count)
      {
         // abort query
         return false;
      }

      return true;
   }
};

void BubbleCube::updatePopOnCollisionCondition()
{
   // make the bubble pop when it's moved into another body
   auto countBodies = [this]() -> size_t
   {
      BubbleQueryCallback query_callback;
      query_callback._max_count = _colliding_body_count;
      query_callback._body = _body;
      b2AABB aabb;
      _fixture->GetShape()->ComputeAABB(&aabb, _body->GetTransform(), 0);
      Level::getCurrentLevel()->getWorld()->QueryAABB(&query_callback, aabb);
      return query_callback._bodies.size();
   };

   // this is going to be the reference count of bodies for future checks
   if (!_colliding_body_count.has_value())
   {
      _colliding_body_count = countBodies();
   }

   // check for collisions with surrounding areas
   if (!_popped && _move_down_on_contact && (_foot_sensor_triggered_counter > 0))
   {
      if (countBodies() > _colliding_body_count)
      {
         _collided_with_surrounding_areas = true;
      }
   }
}

void BubbleCube::update(const sf::Time& dt)
{
   _elapsed_s += dt.asSeconds();
   _pop_elapsed_s += dt.asSeconds();

   updateMotorSpeed(dt);
   updateFootSensorContact();
   updatePosition();
   updateMaxDurationCondition(dt);
   updatePopOnCollisionCondition();
   updatePoppedCondition();
   updateRespawnCondition();
   updateJumpedOffPlatformCondition();
}

std::optional<sf::FloatRect> BubbleCube::getBoundingBoxPx()
{
   return _fixed_rect_px;
}

void BubbleCube::pop()
{
   _popped = true;
   _pop_time = GlobalClock::getInstance().getElapsedTime();
   _pop_elapsed_s = 0.0f;

   _lost_foot_contact = false;
   _foot_sensor_triggered_counter = 0;
   _exceeded_max_contact_duration = false;
   _collided_with_surrounding_areas = false;
   _jumped_off_this_platform = false;
   _motor_time_s = 0.0f;
   _motor_speed = 0.0f;
   _foot_sensor_rect_intersects = false;
   _body->SetEnabled(false);
}

// the box2d code path is no longer required and only kept for debugging purposes

void BubbleCube::beginContact(b2Contact* /*contact*/, FixtureNode* other)
{
   if (other->getType() != ObjectTypePlayerFootSensor)
   {
      return;
   }

   if (_popped)
   {
      return;
   }

   _foot_sensor_contact = true;
}

void BubbleCube::endContact(FixtureNode* other)
{
   if (other->getType() != ObjectTypePlayerFootSensor)
   {
      return;
   }

   if (_popped)
   {
      return;
   }

   _foot_sensor_contact = false;
}
