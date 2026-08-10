#include "playerharpoon.h"

#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/io/texturepool.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace
{
constexpr auto rope_segment_length_m = 0.3f;
constexpr auto rope_half_thickness_m = 0.02f;
constexpr auto rope_length_max_m = 7.0f;
constexpr auto rope_length_min_m = 0.5f;
constexpr auto rope_segment_count_min = 2;
constexpr auto reel_speed_in_mps = 1.6f;
constexpr auto reel_speed_out_mps = 2.2f;
constexpr auto reel_pull_acceleration = 14.0f;

// the rope can be paid out further than the shot reached, so climbing down into a pit works
constexpr auto rope_length_reeled_max_m = 10.0f;
constexpr auto hook_speed_mps = 26.0f;
constexpr auto player_to_segment_mass_ratio = 3.0f;
constexpr auto swing_control_acceleration = 6.0f;
constexpr auto release_grace_duration_s = 0.6f;
constexpr auto player_attachment_offset_m = -0.25f;
constexpr auto rope_draw_half_thickness_px = 0.025f * PPM;  // same as Rope::draw

// the hook sprite is centered on its shank, this pulls it back so the point ends up in the surface
constexpr auto hook_tip_offset_px = 3.0f;

const auto flat_normal_color = sf::Color{128, 128, 255};

// the rope must not collide with the player: the player's solid fixtures collide with
// CategoryBoundary, CategoryEnemyCollideWith and CategoryMoveableBox, its sensors additionally with
// CategoryEnemyWalkThrough. CategoryNoCastShadow appears in none of those masks, so the rope only
// ever touches the level geometry - which is what makes it wrap around corners.
constexpr uint16_t rope_category_bits = CategoryNoCastShadow;
constexpr uint16_t rope_mask_bits = CategoryBoundary;

class HarpoonRayCastCallback : public b2RayCastCallback
{
public:
   HarpoonRayCastCallback(b2Body* ignored_body) : _ignored_body(ignored_body)
   {
   }

   float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
   {
      if (fixture->GetBody() == _ignored_body)
      {
         return -1.0f;
      }

      if (fixture->IsSensor())
      {
         return -1.0f;
      }

      if ((fixture->GetFilterData().categoryBits & CategoryBoundary) == 0)
      {
         return -1.0f;
      }

      _impact_point = point;
      _impact_normal = normal;
      _impact_fraction = fraction;

      return fraction;
   }

   b2Vec2 _impact_point{};
   b2Vec2 _impact_normal{};
   std::optional<float> _impact_fraction;
   b2Body* _ignored_body{nullptr};
};

}  // namespace

void PlayerHarpoon::update(const sf::Time& dt, const HarpoonInput& input)
{
   const auto harpoon_button_just_pressed = input._harpoon_button_pressed && !_harpoon_button_was_pressed;
   _harpoon_button_was_pressed = input._harpoon_button_pressed;

   const auto jump_button_just_pressed = input._jump_button_pressed && !_jump_button_was_pressed;
   _jump_button_was_pressed = input._jump_button_pressed;

   if (input._dead)
   {
      reset();
      return;
   }

   // going into the water costs the player the rope: the harpoon cannot be fired while swimming, so
   // keeping an already attached one would be the odd case out
   if (input._in_water && _state != State::Idle)
   {
      reset();
      return;
   }

   if (_release_grace_remaining_s > 0.0f)
   {
      _release_grace_remaining_s -= dt.asSeconds();

      // the momentum kept from a swing is only interesting while the player is still flying
      if (input._on_ground)
      {
         _release_grace_remaining_s = 0.0f;
      }
   }

   switch (_state)
   {
      case State::Idle:
      {
         if (harpoon_button_just_pressed && !input._in_water)
         {
            shoot(input);
         }
         break;
      }

      case State::Flying:
      {
         _flight_travelled_m += hook_speed_mps * dt.asSeconds();

         if (harpoon_button_just_pressed)
         {
            _state = State::Idle;
            break;
         }

         if (_flight_travelled_m >= _flight_length_m)
         {
            if (_target_found)
            {
               createRope(input);
            }
            else
            {
               _state = State::Idle;
            }
         }
         break;
      }

      case State::Attached:
      {
         if (harpoon_button_just_pressed || jump_button_just_pressed)
         {
            release();
            break;
         }

         updateRopeLength(dt, input);
         applySwingControl(input);
         break;
      }
   }
}

b2Vec2 PlayerHarpoon::readShootDirection(const HarpoonInput& input) const
{
   const auto horizontal = input._points_to_left ? -1.0f : 1.0f;

   // shooting diagonally up is the direction a swing is started from, up and forward are the two
   // useful variations
   auto direction = b2Vec2{horizontal, -1.0f};

   if (input._up_pressed)
   {
      direction = b2Vec2{0.0f, -1.0f};
   }
   else if (input._down_pressed)
   {
      direction = b2Vec2{horizontal, 0.0f};
   }

   direction.Normalize();
   return direction;
}

b2Vec2 PlayerHarpoon::readPlayerAttachmentPosition(b2Body* player_body) const
{
   return player_body->GetPosition() + b2Vec2{0.0f, player_attachment_offset_m};
}

void PlayerHarpoon::shoot(const HarpoonInput& input)
{
   if (!input._player_body || !input._world)
   {
      return;
   }

   _shoot_position_m = readPlayerAttachmentPosition(input._player_body);
   _shoot_direction_m = readShootDirection(input);

   const auto ray_end_m = _shoot_position_m + rope_length_max_m * _shoot_direction_m;

   HarpoonRayCastCallback callback{input._player_body};
   input._world->RayCast(&callback, _shoot_position_m, ray_end_m);

   _target_found = callback._impact_fraction.has_value();
   _anchor_position_m = _target_found ? callback._impact_point : ray_end_m;
   _flight_length_m = (_anchor_position_m - _shoot_position_m).Length();
   _flight_travelled_m = 0.0f;

   // a hook that bites right next to the player leaves no rope to swing on
   if (_flight_length_m < rope_length_min_m)
   {
      _target_found = false;
   }

   _state = State::Flying;
}

void PlayerHarpoon::createRope(const HarpoonInput& input)
{
   if (!input._player_body || !input._world)
   {
      _state = State::Idle;
      return;
   }

   const auto player_attachment_m = readPlayerAttachmentPosition(input._player_body);
   const auto rope_vector_m = player_attachment_m - _anchor_position_m;
   const auto rope_length_m = rope_vector_m.Length();

   if (rope_length_m < rope_length_min_m)
   {
      _state = State::Idle;
      return;
   }

   _world = input._world;

   const auto segment_count = std::max(static_cast<int32_t>(std::round(rope_length_m / rope_segment_length_m)), rope_segment_count_min);
   _segment_length_m = rope_length_m / static_cast<float>(segment_count);
   _player_link_length_m = 0.0f;

   // box2d resolves a chain that carries a much heavier body with visible stretch, so the segment
   // mass is derived from the player mass instead of from a fixed density
   const auto segment_area = _segment_length_m * 2.0f * rope_half_thickness_m;
   _segment_density = (input._player_body->GetMass() / player_to_segment_mass_ratio) / segment_area;

   auto rope_direction_m = rope_vector_m;
   rope_direction_m.Normalize();
   const auto segment_angle = std::atan2(rope_direction_m.y, rope_direction_m.x);

   b2BodyDef anchor_body_def;
   anchor_body_def.type = b2_staticBody;
   anchor_body_def.position = _anchor_position_m;
   _anchor_body = _world->CreateBody(&anchor_body_def);

   auto* previous_body = _anchor_body;

   for (auto segment_index = 0; segment_index < segment_count; segment_index++)
   {
      // the first segment sits inside the surface the hook is stuck in; letting it collide would
      // push the whole rope back out of the wall
      auto* segment_body = createSegment(
         _anchor_position_m + ((static_cast<float>(segment_index) + 0.5f) * _segment_length_m) * rope_direction_m,
         segment_angle,
         segment_index > 0
      );

      b2RevoluteJointDef joint_def;
      joint_def.Initialize(
         previous_body, segment_body, _anchor_position_m + (static_cast<float>(segment_index) * _segment_length_m) * rope_direction_m
      );
      joint_def.collideConnected = false;
      _rope_joints.push_back(_world->CreateJoint(&joint_def));

      _rope_bodies.push_back(segment_body);
      previous_body = segment_body;
   }

   attachPlayer(input._player_body);

   _state = State::Attached;

   Audio::getInstance().playSample({"arrow_hit_1.ogg", 0.5f});
}

b2Body* PlayerHarpoon::createSegment(const b2Vec2& center_m, float angle, bool colliding)
{
   b2BodyDef segment_body_def;
   segment_body_def.type = b2_dynamicBody;
   segment_body_def.position = center_m;
   segment_body_def.angle = angle;
   segment_body_def.linearDamping = 0.1f;
   segment_body_def.angularDamping = 0.1f;
   auto* segment_body = _world->CreateBody(&segment_body_def);

   b2PolygonShape segment_shape;
   segment_shape.SetAsBox(_segment_length_m * 0.5f, rope_half_thickness_m);

   b2FixtureDef segment_fixture_def;
   segment_fixture_def.shape = &segment_shape;
   segment_fixture_def.density = _segment_density;
   segment_fixture_def.friction = 0.2f;
   segment_fixture_def.filter.categoryBits = rope_category_bits;
   segment_fixture_def.filter.maskBits = colliding ? rope_mask_bits : 0;
   segment_body->CreateFixture(&segment_fixture_def);

   return segment_body;
}

void PlayerHarpoon::attachPlayer(b2Body* player_body)
{
   // this last link is what makes reeling smooth: it is a rope-style distance joint whose maximum
   // length can be changed every frame without recreating the joint, so the rope grows and shrinks
   // continuously and whole segments are only added or removed when the link passes a segment
   // boundary. its local anchors are set explicitly rather than through Initialize(), which derives
   // them from the current positions and would bake the current gap into the joint - leaving a
   // constraint that is already satisfied and never pulls the player anywhere.
   b2DistanceJointDef player_joint_def;
   player_joint_def.bodyA = _rope_bodies.back();
   player_joint_def.bodyB = player_body;
   player_joint_def.localAnchorA = b2Vec2{_segment_length_m * 0.5f, 0.0f};
   player_joint_def.localAnchorB = b2Vec2{0.0f, player_attachment_offset_m};
   player_joint_def.length = _player_link_length_m;
   player_joint_def.minLength = 0.0f;
   player_joint_def.maxLength = _player_link_length_m;

   // a stiffness of zero keeps the limits rigid and skips the spring, which is rope behaviour:
   // the player can come closer than the link length but never further away
   player_joint_def.stiffness = 0.0f;
   player_joint_def.damping = 0.0f;
   player_joint_def.collideConnected = false;

   _player_joint = static_cast<b2DistanceJoint*>(_world->CreateJoint(&player_joint_def));
}

float PlayerHarpoon::readRopeLength() const
{
   return static_cast<float>(_rope_bodies.size()) * _segment_length_m + _player_link_length_m;
}

void PlayerHarpoon::updateRopeLength(const sf::Time& dt, const HarpoonInput& input)
{
   if (!_world || _rope_bodies.empty() || !_player_joint)
   {
      return;
   }

   // up and down cancel each other out
   if (input._up_pressed == input._down_pressed)
   {
      return;
   }

   const auto reeling_in = input._up_pressed;
   const auto rope_length_m = readRopeLength();

   if (reeling_in && rope_length_m <= rope_length_min_m)
   {
      return;
   }

   if (!reeling_in && rope_length_m >= rope_length_reeled_max_m)
   {
      return;
   }

   // shortening the link alone does not lift the player: the chain carries a body several times the
   // mass of one segment, so box2d pulls the light rope end down instead of hoisting him. the pull
   // along the rope is what turns reeling in into climbing.
   if (reeling_in)
   {
      pullPlayerAlongRope(input);
   }

   const auto reel_speed_mps = reeling_in ? -reel_speed_in_mps : reel_speed_out_mps;
   _player_link_length_m += reel_speed_mps * dt.asSeconds();

   // the link only ever spans one segment; crossing that boundary converts it into a whole segment
   if (_player_link_length_m < 0.0f)
   {
      if (static_cast<int32_t>(_rope_bodies.size()) > rope_segment_count_min)
      {
         removeLastSegment(input);
         _player_link_length_m += _segment_length_m;
      }
      else
      {
         _player_link_length_m = 0.0f;
      }
   }
   else if (_player_link_length_m > _segment_length_m)
   {
      appendSegment(input);
      _player_link_length_m -= _segment_length_m;
   }

   _player_joint->SetLength(_player_link_length_m);
   _player_joint->SetMaxLength(_player_link_length_m);
}

void PlayerHarpoon::pullPlayerAlongRope(const HarpoonInput& input)
{
   auto pull_direction_m = _rope_bodies.back()->GetPosition() - readPlayerAttachmentPosition(input._player_body);

   if (pull_direction_m.LengthSquared() < 0.0001f)
   {
      return;
   }

   pull_direction_m.Normalize();

   const auto force = reel_pull_acceleration * input._player_body->GetMass();
   input._player_body->ApplyForceToCenter(force * pull_direction_m, true);
}

void PlayerHarpoon::removeLastSegment(const HarpoonInput& input)
{
   _world->DestroyJoint(_player_joint);
   _player_joint = nullptr;

   // the joint that held the removed segment goes with it
   _world->DestroyJoint(_rope_joints.back());
   _rope_joints.pop_back();

   _world->DestroyBody(_rope_bodies.back());
   _rope_bodies.pop_back();

   attachPlayer(input._player_body);
}

void PlayerHarpoon::appendSegment(const HarpoonInput& input)
{
   auto* previous_body = _rope_bodies.back();
   const auto joint_position_m = previous_body->GetWorldPoint(b2Vec2{_segment_length_m * 0.5f, 0.0f});

   auto segment_direction_m = readPlayerAttachmentPosition(input._player_body) - joint_position_m;
   if (segment_direction_m.LengthSquared() < 0.0001f)
   {
      return;
   }
   segment_direction_m.Normalize();

   _world->DestroyJoint(_player_joint);
   _player_joint = nullptr;

   auto* segment_body = createSegment(
      joint_position_m + (_segment_length_m * 0.5f) * segment_direction_m, std::atan2(segment_direction_m.y, segment_direction_m.x), true
   );

   // inheriting the player velocity keeps the rope from jerking when it grows
   segment_body->SetLinearVelocity(input._player_body->GetLinearVelocity());

   b2RevoluteJointDef joint_def;
   joint_def.Initialize(previous_body, segment_body, joint_position_m);
   joint_def.collideConnected = false;
   _rope_joints.push_back(_world->CreateJoint(&joint_def));

   _rope_bodies.push_back(segment_body);

   attachPlayer(input._player_body);
}

void PlayerHarpoon::applySwingControl(const HarpoonInput& input)
{
   auto direction = 0.0f;

   if (input._move_left_pressed)
   {
      direction -= 1.0f;
   }

   if (input._move_right_pressed)
   {
      direction += 1.0f;
   }

   if (fabs(direction) < 0.01f || _rope_bodies.empty())
   {
      return;
   }

   // a force pointing along the rope only adds tension, it does not make the player swing faster.
   // projecting the requested direction onto the tangent of the arc puts all of it into the swing:
   // full effect at the bottom of the arc, nothing at the point where the rope is horizontal and
   // pulling sideways would just stretch it. the pivot is taken from the rope segment closest to the
   // player, so this keeps working while the rope is wrapped around a corner.
   auto rope_direction_m = _rope_bodies.back()->GetPosition() - input._player_body->GetPosition();
   rope_direction_m.Normalize();

   const auto tangent_m = b2Vec2{-rope_direction_m.y, rope_direction_m.x};
   const auto tangent_share = direction * tangent_m.x;
   const auto force = tangent_share * swing_control_acceleration * input._player_body->GetMass();

   input._player_body->ApplyForceToCenter(force * tangent_m, true);
}

void PlayerHarpoon::release()
{
   destroyRope();
   _release_grace_remaining_s = release_grace_duration_s;
   _state = State::Idle;
}

void PlayerHarpoon::destroyRope()
{
   if (_world)
   {
      // the joints are destroyed first, destroying a body would take its joints with it
      if (_player_joint)
      {
         _world->DestroyJoint(_player_joint);
      }

      for (auto* joint : _rope_joints)
      {
         _world->DestroyJoint(joint);
      }

      for (auto* body : _rope_bodies)
      {
         _world->DestroyBody(body);
      }

      if (_anchor_body)
      {
         _world->DestroyBody(_anchor_body);
      }
   }

   _player_joint = nullptr;
   _rope_joints.clear();
   _rope_bodies.clear();
   _anchor_body = nullptr;
   _world.reset();
}

void PlayerHarpoon::reset()
{
   destroyRope();

   _state = State::Idle;
   _release_grace_remaining_s = 0.0f;
   _harpoon_button_was_pressed = false;
   _jump_button_was_pressed = false;
   _target_found = false;
   _flight_length_m = 0.0f;
   _flight_travelled_m = 0.0f;
   _player_link_length_m = 0.0f;
}

bool PlayerHarpoon::isAttached() const
{
   return _state == State::Attached;
}

bool PlayerHarpoon::isReleaseGraceActive() const
{
   return _release_grace_remaining_s > 0.0f;
}

void PlayerHarpoon::draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   loadTextures();

   if (_state == State::Idle)
   {
      return;
   }

   // the rope is drawn from the hook towards the player
   std::vector<b2Vec2> rope_points_m;

   if (_state == State::Flying)
   {
      rope_points_m.push_back(_shoot_position_m + _flight_travelled_m * _shoot_direction_m);
      rope_points_m.push_back(_shoot_position_m);
   }
   else
   {
      rope_points_m.push_back(_anchor_position_m);

      for (auto* body : _rope_bodies)
      {
         rope_points_m.push_back(body->GetPosition());
      }

      if (_player_joint)
      {
         rope_points_m.push_back(_player_joint->GetAnchorB());
      }
   }

   if (rope_points_m.size() < 2)
   {
      return;
   }

   const auto rope_texture_width_px = static_cast<float>(_rope_texture->getSize().x);

   std::vector<sf::Vertex> rope_strip;
   std::optional<sf::Vector2f> previous_left_px;
   std::optional<sf::Vector2f> previous_right_px;
   auto travelled_px = 0.0f;

   for (auto point_index = 0u; point_index < rope_points_m.size() - 1; point_index++)
   {
      const auto current_px = sf::Vector2f{rope_points_m[point_index].x * PPM, rope_points_m[point_index].y * PPM};
      const auto next_px = sf::Vector2f{rope_points_m[point_index + 1].x * PPM, rope_points_m[point_index + 1].y * PPM};

      auto segment_normal_px = sf::Vector2f{next_px.y - current_px.y, current_px.x - next_px.x};
      const auto segment_length_px = std::sqrt((segment_normal_px.x * segment_normal_px.x) + (segment_normal_px.y * segment_normal_px.y));

      if (segment_length_px < 0.0001f)
      {
         continue;
      }

      segment_normal_px = (rope_draw_half_thickness_px / segment_length_px) * segment_normal_px;

      const auto current_left_px = previous_left_px.value_or(current_px - segment_normal_px);
      const auto current_right_px = previous_right_px.value_or(current_px + segment_normal_px);
      const auto next_left_px = next_px - segment_normal_px;
      const auto next_right_px = next_px + segment_normal_px;

      previous_left_px = next_left_px;
      previous_right_px = next_right_px;

      // the texture repeats along the rope, so the v coordinate is the distance travelled so far
      const auto v_from = travelled_px;
      travelled_px += segment_length_px;
      const auto v_to = travelled_px;

      rope_strip.push_back(sf::Vertex{current_left_px, sf::Color::White, sf::Vector2f{0.0f, v_from}});
      rope_strip.push_back(sf::Vertex{next_left_px, sf::Color::White, sf::Vector2f{0.0f, v_to}});
      rope_strip.push_back(sf::Vertex{current_right_px, sf::Color::White, sf::Vector2f{rope_texture_width_px, v_from}});
      rope_strip.push_back(sf::Vertex{next_right_px, sf::Color::White, sf::Vector2f{rope_texture_width_px, v_to}});
   }

   auto rope_states = states;
   rope_states.texture = _rope_texture.get();

   auto draw_strip = [](sf::RenderTarget& target, const std::vector<sf::Vertex>& strip, const sf::RenderStates& strip_states)
   {
      if (strip.empty())
      {
         return;
      }

#ifdef __EMSCRIPTEN__
      target.draw(std::span<const sf::Vertex>{strip.data(), strip.size()}, sf::PrimitiveType::TriangleStrip, strip_states);
#else
      target.draw(strip.data(), strip.size(), sf::PrimitiveType::TriangleStrip, strip_states);
#endif
   };

   draw_strip(color, rope_strip, rope_states);

   // the rope has no normal map of its own, a flat normal over the same geometry keeps it from
   // picking up the wall normals behind it
   auto flat_normal_states = states;
   flat_normal_states.texture = nullptr;
   auto normal_strip = rope_strip;
   for (auto& vertex : normal_strip)
   {
      vertex.color = flat_normal_color;
   }
   draw_strip(normal, normal_strip, flat_normal_states);

   // the hook sits at the anchor, pulled back along the shot direction so its point is the part
   // buried in the surface, and turned so its shank follows the rope back to the player
   const auto hook_position_px =
      sf::Vector2f{rope_points_m.front().x * PPM, rope_points_m.front().y * PPM} - hook_tip_offset_px * toVector2f(_shoot_direction_m);
   const auto hook_strip = buildSpriteStrip(*_hook_texture, hook_position_px, _shoot_direction_m, false);

   auto hook_states = states;
   hook_states.texture = _hook_texture.get();
   draw_strip(color, hook_strip, hook_states);
}

sf::Vector2f PlayerHarpoon::toVector2f(const b2Vec2& vector)
{
   return sf::Vector2f{vector.x, vector.y};
}

std::vector<sf::Vertex>
PlayerHarpoon::buildSpriteStrip(const sf::Texture& texture, const sf::Vector2f& center_px, const b2Vec2& direction, bool mirrored)
{
   const auto width_px = static_cast<float>(texture.getSize().x);
   const auto height_px = static_cast<float>(texture.getSize().y);
   const auto half_width_px = width_px * 0.5f;
   const auto half_height_px = height_px * 0.5f;

   // the sprites are drawn pointing along +x, so the direction vector is the rotation
   const auto along_px = sf::Vector2f{direction.x, direction.y};
   auto across_px = sf::Vector2f{-direction.y, direction.x};

   // mirroring negates the perpendicular axis instead of swapping the texture coordinates: turning a
   // left facing sprite around by 180 degrees would put its bottom on top
   if (mirrored)
   {
      across_px = -across_px;
   }

   const auto top_left_px = center_px - half_width_px * along_px - half_height_px * across_px;
   const auto bottom_left_px = center_px - half_width_px * along_px + half_height_px * across_px;
   const auto top_right_px = center_px + half_width_px * along_px - half_height_px * across_px;
   const auto bottom_right_px = center_px + half_width_px * along_px + half_height_px * across_px;

   return {
      sf::Vertex{top_left_px, sf::Color::White, sf::Vector2f{0.0f, 0.0f}},
      sf::Vertex{bottom_left_px, sf::Color::White, sf::Vector2f{0.0f, height_px}},
      sf::Vertex{top_right_px, sf::Color::White, sf::Vector2f{width_px, 0.0f}},
      sf::Vertex{bottom_right_px, sf::Color::White, sf::Vector2f{width_px, height_px}},
   };
}

void PlayerHarpoon::loadTextures()
{
   if (_rope_texture)
   {
      return;
   }

   _rope_texture = TexturePool::getInstance().get("data/sprites/harpoon_rope.png");
   _hook_texture = TexturePool::getInstance().get("data/sprites/harpoon_hook.png");

   // the rope texture tiles along the rope, its v coordinate runs over the whole rope length
#ifdef __EMSCRIPTEN__
   _rope_texture->setWrapMode(sf::TextureWrapMode::Repeat);
#else
   _rope_texture->setRepeated(true);
#endif
}
