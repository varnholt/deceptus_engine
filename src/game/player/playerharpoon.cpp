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

// aiming: the angle is held relative to the facing direction, positive points up. digital input
// sweeps it, an analogue stick sets it directly. the default is the diagonal a swing starts from.
constexpr auto aim_angle_default_deg = 45.0f;
constexpr auto aim_angle_min_deg = -30.0f;
constexpr auto aim_angle_max_deg = 100.0f;
constexpr auto aim_sweep_speed_deg_per_s = 120.0f;
constexpr auto aim_deadzone = 0.35f;

// the indicator: a few dots along the aim ray, fading out. no animation, the angle is the message
constexpr auto aim_dot_count = 5;
constexpr auto aim_dot_first_px = 11.0f;
constexpr auto aim_dot_spacing_px = 7.0f;
constexpr auto aim_dot_half_size_px = 1.0f;
constexpr auto aim_dot_alpha_max = 190.0f;

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
         updateAiming(dt, input);
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

            // releasing with the fire button must not roll straight into a new aim
            _fire_locked_until_released = input._harpoon_button_pressed;
            break;
         }

         updateRopeLength(dt, input);
         applySwingControl(input);
         break;
      }
   }

   updateVerticalKeyClaim(input);
}

void PlayerHarpoon::updateVerticalKeyClaim(const HarpoonInput& input)
{
   _hold.syncVerticalKeyClaim(input._controls, _aiming || _state == State::Attached);
}

void PlayerHarpoon::updateAiming(const sf::Time& dt, const HarpoonInput& input)
{
   if (!input._player_body)
   {
      return;
   }

   // after the rope was dropped with the fire button, wait for that button to come up again
   if (_fire_locked_until_released)
   {
      _fire_locked_until_released = input._harpoon_button_pressed;
      return;
   }

   if (input._harpoon_button_pressed && !input._in_water && !input._carried_elsewhere)
   {
      if (!_aiming)
      {
         // up and down are the aim, so an aim cannot start while they belong to something else
         if (_hold.areVerticalKeysOwnedElsewhere(input._controls))
         {
            return;
         }

         // every aim starts from the default, which keeps the angle predictable
         _aiming = true;
         _aim_angle_deg = aim_angle_default_deg;

         // claimed right here rather than at the end of the frame, so the aim can read its own keys
         // on the very frame it starts instead of sitting at zero for one frame
         updateVerticalKeyClaim(input);
      }

      // the stick is only read when a controller is actually the input device
      const auto stick =
         input._analogue_aim ? sf::Vector2f{_hold.readHorizontalAxis(), _hold.readVerticalAxis()} : sf::Vector2f{};
      const auto analogue_aim = (stick.x * stick.x) + (stick.y * stick.y) > aim_deadzone * aim_deadzone;

      if (analogue_aim)
      {
         // an analogue stick points at the angle directly, no sweeping needed
         const auto forward = input._points_to_left ? -stick.x : stick.x;
         _aim_angle_deg = std::atan2(-stick.y, forward) * FACTOR_RAD_TO_DEG;
      }
      else
      {
         // digital input sweeps the angle, which is what puts arbitrary angles within its reach
         const auto sweep_deg = aim_sweep_speed_deg_per_s * dt.asSeconds();
         _aim_angle_deg += _hold.isUpPressed() ? sweep_deg : 0.0f;
         _aim_angle_deg -= _hold.isDownPressed() ? sweep_deg : 0.0f;
      }

      _aim_angle_deg = std::clamp(_aim_angle_deg, aim_angle_min_deg, aim_angle_max_deg);

      // screen space y grows downwards, so a positive aim angle subtracts
      const auto angle_rad = _aim_angle_deg * FACTOR_DEG_TO_RAD;
      const auto forward = input._points_to_left ? -1.0f : 1.0f;
      _aim_direction = b2Vec2{forward * std::cos(angle_rad), -std::sin(angle_rad)};
      _aim_direction.Normalize();

      const auto attachment_m = PlayerRopeHold::readPlayerAttachmentPosition(input._player_body);
      _aim_origin_px = sf::Vector2f{attachment_m.x * PPM, attachment_m.y * PPM};
      return;
   }

   // the shot leaves when the button comes up
   if (_aiming)
   {
      _aiming = false;
      shoot(input);
   }
}

void PlayerHarpoon::drawAimIndicator(sf::RenderTarget& color, const sf::RenderStates& states)
{
   if (!_aiming)
   {
      return;
   }

   std::vector<sf::Vertex> dots;

   for (auto dot_index = 0; dot_index < aim_dot_count; dot_index++)
   {
      const auto distance_px = aim_dot_first_px + static_cast<float>(dot_index) * aim_dot_spacing_px;
      const auto center_px = _aim_origin_px + distance_px * toVector2f(_aim_direction);

      // fading out along the ray is what gives the dots a direction without animating them
      const auto fade = 1.0f - static_cast<float>(dot_index) / static_cast<float>(aim_dot_count);
      const auto dot_color = sf::Color{255, 255, 255, static_cast<uint8_t>(aim_dot_alpha_max * fade)};

      const auto top_left_px = center_px + sf::Vector2f{-aim_dot_half_size_px, -aim_dot_half_size_px};
      const auto bottom_left_px = center_px + sf::Vector2f{-aim_dot_half_size_px, aim_dot_half_size_px};
      const auto top_right_px = center_px + sf::Vector2f{aim_dot_half_size_px, -aim_dot_half_size_px};
      const auto bottom_right_px = center_px + sf::Vector2f{aim_dot_half_size_px, aim_dot_half_size_px};

      // one triangle strip per dot would need one draw call each, so they go out as triangles
      dots.push_back(sf::Vertex{top_left_px, dot_color, sf::Vector2f{}});
      dots.push_back(sf::Vertex{bottom_left_px, dot_color, sf::Vector2f{}});
      dots.push_back(sf::Vertex{top_right_px, dot_color, sf::Vector2f{}});
      dots.push_back(sf::Vertex{bottom_left_px, dot_color, sf::Vector2f{}});
      dots.push_back(sf::Vertex{bottom_right_px, dot_color, sf::Vector2f{}});
      dots.push_back(sf::Vertex{top_right_px, dot_color, sf::Vector2f{}});
   }

   auto dot_states = states;
   dot_states.texture = nullptr;

#ifdef __EMSCRIPTEN__
   color.draw(std::span<const sf::Vertex>{dots.data(), dots.size()}, sf::PrimitiveType::Triangles, dot_states);
#else
   color.draw(dots.data(), dots.size(), sf::PrimitiveType::Triangles, dot_states);
#endif
}

void PlayerHarpoon::shoot(const HarpoonInput& input)
{
   if (!input._player_body || !input._world)
   {
      return;
   }

   _shoot_position_m = PlayerRopeHold::readPlayerAttachmentPosition(input._player_body);
   _shoot_direction_m = _aim_direction;

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

   const auto player_attachment_m = PlayerRopeHold::readPlayerAttachmentPosition(input._player_body);
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
   // the anchor sits at the far end of the last segment, which is where the rope actually ends
   _hold.attach(_world, _rope_bodies.back(), player_body, b2Vec2{_segment_length_m * 0.5f, 0.0f}, _player_link_length_m);
}

float PlayerHarpoon::readRopeLength() const
{
   return static_cast<float>(_rope_bodies.size()) * _segment_length_m + _player_link_length_m;
}

void PlayerHarpoon::updateRopeLength(const sf::Time& dt, const HarpoonInput& input)
{
   if (!_world || _rope_bodies.empty() || !_hold.isAttached())
   {
      return;
   }

   // up and down cancel each other out
   const auto up_pressed = _hold.isUpPressed();
   const auto down_pressed = _hold.isDownPressed();

   if (up_pressed == down_pressed)
   {
      return;
   }

   const auto reeling_in = up_pressed;
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
      _hold.pullPlayerTowards(input._player_body, _rope_bodies.back()->GetPosition(), reel_pull_acceleration);
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

   _hold.setLinkLength(_player_link_length_m);
}

void PlayerHarpoon::removeLastSegment(const HarpoonInput& input)
{
   _hold.detach();

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

   auto segment_direction_m = PlayerRopeHold::readPlayerAttachmentPosition(input._player_body) - joint_position_m;
   if (segment_direction_m.LengthSquared() < 0.0001f)
   {
      return;
   }
   segment_direction_m.Normalize();

   _hold.detach();

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

   _hold.applySwingControl(input._player_body, direction, swing_control_acceleration);
}

void PlayerHarpoon::release()
{
   destroyRope();
   _release_grace_remaining_s = release_grace_duration_s;
   _state = State::Idle;
}

void PlayerHarpoon::destroyRope()
{
   // the joints are destroyed first, destroying a body would take its joints with it
   _hold.detach();

   if (_world)
   {
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
   _aiming = false;
   _fire_locked_until_released = false;
   _target_found = false;
   _flight_length_m = 0.0f;
   _flight_travelled_m = 0.0f;
   _player_link_length_m = 0.0f;
   _hold.syncVerticalKeyClaim({}, false);
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
   drawAimIndicator(color, states);

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

      if (const auto player_anchor_m = _hold.readPlayerAnchorPosition())
      {
         rope_points_m.push_back(player_anchor_m.value());
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
