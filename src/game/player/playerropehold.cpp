#include "playerropehold.h"

#include <cmath>

namespace
{
constexpr auto player_attachment_offset_m = -0.25f;

// the deadzone the vertical axis has to leave before up or down count as pressed, matching the default
// analog threshold of the regular movement queries
constexpr auto vertical_axis_threshold = 0.3f;
}  // namespace

void PlayerRopeHold::attach(
   const std::shared_ptr<b2World>& world,
   b2Body* rope_body,
   b2Body* player_body,
   const b2Vec2& local_anchor_rope,
   float link_length_m
)
{
   destroyJoint();

   _world = world;
   _rope_body = rope_body;
   _player_body = player_body;
   _local_anchor_rope = local_anchor_rope;
   _link_length_m = link_length_m;

   createJoint();
}

void PlayerRopeHold::detach()
{
   destroyAnchorLimit();
   destroyJoint();

   _rope_body = nullptr;
   _player_body = nullptr;
   _world.reset();
}

void PlayerRopeHold::setAnchorLimit(b2Body* anchor_body, const b2Vec2& local_anchor, float max_distance_m)
{
   destroyAnchorLimit();

   if (!_world || !_player_body || !anchor_body)
   {
      return;
   }

   b2DistanceJointDef anchor_joint_def;
   anchor_joint_def.bodyA = anchor_body;
   anchor_joint_def.bodyB = _player_body;
   anchor_joint_def.localAnchorA = local_anchor;
   anchor_joint_def.localAnchorB = b2Vec2{0.0f, player_attachment_offset_m};

   // a length of zero with a maximum keeps this from ever pulling: it is a limit, not a spring
   anchor_joint_def.length = 0.0f;
   anchor_joint_def.minLength = 0.0f;
   anchor_joint_def.maxLength = max_distance_m;
   anchor_joint_def.stiffness = 0.0f;
   anchor_joint_def.damping = 0.0f;
   anchor_joint_def.collideConnected = false;

   _anchor_limit_joint = static_cast<b2DistanceJoint*>(_world->CreateJoint(&anchor_joint_def));
}

void PlayerRopeHold::setAnchorLimitDistance(float max_distance_m)
{
   if (!_anchor_limit_joint)
   {
      return;
   }

   _anchor_limit_joint->SetMaxLength(max_distance_m);
}

void PlayerRopeHold::destroyAnchorLimit()
{
   if (_world && _anchor_limit_joint)
   {
      _world->DestroyJoint(_anchor_limit_joint);
   }

   _anchor_limit_joint = nullptr;
}

void PlayerRopeHold::rebind(b2Body* rope_body, const b2Vec2& local_anchor_rope, float link_length_m)
{
   if (!_world || !_player_body)
   {
      return;
   }

   destroyJoint();

   _rope_body = rope_body;
   _local_anchor_rope = local_anchor_rope;
   _link_length_m = link_length_m;

   createJoint();
}

void PlayerRopeHold::createJoint()
{
   if (!_world || !_rope_body || !_player_body)
   {
      return;
   }

   // this link is what makes climbing and reeling smooth: it is a rope-style distance joint whose
   // maximum length can be changed every frame without recreating the joint, so the rope grows and
   // shrinks continuously and the hold only moves to another rope body when the link passes a segment
   // boundary. its local anchors are set explicitly rather than through Initialize(), which derives
   // them from the current positions and would bake the current gap into the joint - leaving a
   // constraint that is already satisfied and never pulls the player anywhere.
   b2DistanceJointDef player_joint_def;
   player_joint_def.bodyA = _rope_body;
   player_joint_def.bodyB = _player_body;
   player_joint_def.localAnchorA = _local_anchor_rope;
   player_joint_def.localAnchorB = b2Vec2{0.0f, player_attachment_offset_m};
   player_joint_def.length = _link_length_m;
   player_joint_def.minLength = 0.0f;
   player_joint_def.maxLength = _link_length_m;

   // a stiffness of zero keeps the limits rigid and skips the spring, which is rope behaviour:
   // the player can come closer than the link length but never further away
   player_joint_def.stiffness = 0.0f;
   player_joint_def.damping = 0.0f;
   player_joint_def.collideConnected = false;

   _joint = static_cast<b2DistanceJoint*>(_world->CreateJoint(&player_joint_def));
}

void PlayerRopeHold::destroyJoint()
{
   if (_world && _joint)
   {
      _world->DestroyJoint(_joint);
   }

   _joint = nullptr;
}

void PlayerRopeHold::setLinkLength(float link_length_m)
{
   _link_length_m = link_length_m;

   if (!_joint)
   {
      return;
   }

   _joint->SetLength(_link_length_m);
   _joint->SetMaxLength(_link_length_m);
}

float PlayerRopeHold::getLinkLength() const
{
   return _link_length_m;
}

b2Body* PlayerRopeHold::getRopeBody() const
{
   return _rope_body;
}

bool PlayerRopeHold::isAttached() const
{
   return _joint != nullptr;
}

std::optional<b2Vec2> PlayerRopeHold::readPlayerAnchorPosition() const
{
   if (!_joint)
   {
      return std::nullopt;
   }

   return _joint->GetAnchorB();
}

void PlayerRopeHold::applySwingControl(b2Body* player_body, float direction, float acceleration) const
{
   if (!player_body || !_rope_body || fabs(direction) < 0.01f)
   {
      return;
   }

   // a force pointing along the rope only adds tension, it does not make the player swing faster.
   // projecting the requested direction onto the tangent of the arc puts all of it into the swing:
   // full effect at the bottom of the arc, nothing at the point where the rope is horizontal and
   // pulling sideways would just stretch it. the pivot is taken from the rope body the player hangs
   // on, so this keeps working while the rope is wrapped around a corner.
   auto rope_direction_m = _rope_body->GetPosition() - player_body->GetPosition();

   if (rope_direction_m.LengthSquared() < 0.0001f)
   {
      return;
   }

   rope_direction_m.Normalize();

   const auto tangent_m = b2Vec2{-rope_direction_m.y, rope_direction_m.x};
   const auto tangent_share = direction * tangent_m.x;
   const auto force = tangent_share * acceleration * player_body->GetMass();

   player_body->ApplyForceToCenter(force * tangent_m, true);
}

void PlayerRopeHold::pullPlayerTowards(b2Body* player_body, const b2Vec2& target_m, float acceleration) const
{
   if (!player_body)
   {
      return;
   }

   auto pull_direction_m = target_m - readPlayerAttachmentPosition(player_body);

   if (pull_direction_m.LengthSquared() < 0.0001f)
   {
      return;
   }

   pull_direction_m.Normalize();

   const auto force = acceleration * player_body->GetMass();
   player_body->ApplyForceToCenter(force * pull_direction_m, true);
}

void PlayerRopeHold::syncVerticalKeyClaim(const std::shared_ptr<PlayerControls>& controls, bool owns_vertical_keys)
{
   if (owns_vertical_keys == _vertical_key_claim.isActive())
   {
      return;
   }

   if (owns_vertical_keys)
   {
      _vertical_key_claim = KeyClaim{controls->getKeyClaims(), {KeyPressedUp, KeyPressedDown}};
   }
   else
   {
      _vertical_key_claim = {};
   }
}

bool PlayerRopeHold::ownsVerticalKeys() const
{
   return _vertical_key_claim.isActive();
}

bool PlayerRopeHold::areVerticalKeysOwnedElsewhere(const std::shared_ptr<PlayerControls>& controls) const
{
   if (!controls || ownsVerticalKeys())
   {
      return false;
   }

   return controls->getKeyClaims()->isClaimed(KeyPressedUp) || controls->getKeyClaims()->isClaimed(KeyPressedDown);
}

float PlayerRopeHold::readVerticalAxis() const
{
   return _vertical_key_claim.readVerticalAxis();
}

float PlayerRopeHold::readHorizontalAxis() const
{
   return _vertical_key_claim.readHorizontalAxis();
}

bool PlayerRopeHold::isUpPressed() const
{
   return readVerticalAxis() < -vertical_axis_threshold;
}

bool PlayerRopeHold::isDownPressed() const
{
   return readVerticalAxis() > vertical_axis_threshold;
}

b2Vec2 PlayerRopeHold::readPlayerAttachmentPosition(b2Body* player_body)
{
   return player_body->GetPosition() + b2Vec2{0.0f, player_attachment_offset_m};
}
