#include "playerrope.h"

#include "game/audio/audio.h"
#include "game/mechanisms/grabrope.h"
#include "game/mechanisms/grabropewrapper.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr auto climb_speed_up_mps = 1.6f;
constexpr auto climb_speed_down_mps = 2.2f;
constexpr auto climb_pull_acceleration = 14.0f;
constexpr auto swing_control_acceleration = 6.0f;
constexpr auto release_grace_duration_s = 0.6f;
constexpr auto regrab_block_duration_s = 0.4f;

// the chain is allowed to sag a little past its own length before the anchor limit bites, otherwise
// the limit fights the chain constantly and the rope reads as rigid
constexpr auto anchor_limit_slack_m = 0.15f;

// a rope is bolted to something solid, so climbing all the way up to the suspension point would pull
// the player into it. this is roughly his own height, which leaves his head below whatever holds it.
constexpr auto climb_min_distance_from_anchor_m = 0.75f;
}  // namespace

void PlayerRope::update(const sf::Time& dt, const RopeInput& input)
{
   const auto jump_button_just_pressed = input._jump_button_pressed && !_jump_button_was_pressed;
   _jump_button_was_pressed = input._jump_button_pressed;

   if (_regrab_blocked_s > 0.0f)
   {
      _regrab_blocked_s -= dt.asSeconds();
   }

   if (_release_grace_remaining_s > 0.0f)
   {
      _release_grace_remaining_s -= dt.asSeconds();
   }

   if (input._dead || input._in_water)
   {
      reset();
      return;
   }

   if (_hold.isAttached())
   {
      if (jump_button_just_pressed)
      {
         release();
      }
      else
      {
         updateHold(dt, input);
      }
   }
   else
   {
      updateGrab(input);
   }

   _hold.syncVerticalKeyClaim(input._controls, _hold.isAttached());
}

void PlayerRope::updateGrab(const RopeInput& input)
{
   if (!input._player_body || !input._world || !input._in_air || input._carried_elsewhere)
   {
      return;
   }

   if (_regrab_blocked_s > 0.0f)
   {
      return;
   }

   // climbing is up and down, so the rope cannot be grabbed while they belong to something else - the
   // harpoon aiming, for one, would otherwise sweep its angle from the same key that climbs
   if (_hold.areVerticalKeysOwnedElsewhere(input._controls))
   {
      return;
   }

   const auto rope = GrabRopeWrapper::getGrabRopeAt(input._player_rect_px);

   if (!rope || rope->getChainElements().empty())
   {
      return;
   }

   grab(rope, input);
}

void PlayerRope::grab(const std::shared_ptr<GrabRope>& rope, const RopeInput& input)
{
   const auto& chain_elements = rope->getChainElements();
   const auto attachment_m = PlayerRopeHold::readPlayerAttachmentPosition(input._player_body);

   // the element closest to the player is the one he grabs, which is what makes running into a rope
   // half way up catch him there instead of at its end
   auto closest_index = 0;
   auto closest_distance_squared = std::numeric_limits<float>::max();

   for (auto element_index = 0u; element_index < chain_elements.size(); element_index++)
   {
      const auto distance_squared = (chain_elements[element_index]->GetPosition() - attachment_m).LengthSquared();

      if (distance_squared < closest_distance_squared)
      {
         closest_distance_squared = distance_squared;
         closest_index = static_cast<int32_t>(element_index);
      }
   }

   _rope = rope;
   _segment_length_m = rope->getSegmentLength();
   _element_index = closest_index;

   // starting from the distance he already has keeps the grab from yanking him onto the rope
   _link_length_m = std::clamp(std::sqrt(closest_distance_squared), 0.0f, _segment_length_m);

   _hold.attach(input._world, chain_elements[static_cast<size_t>(_element_index)], input._player_body, b2Vec2{0.0f, 0.0f}, _link_length_m);
   _hold.setAnchorLimit(rope->getAnchorBody(), rope->getAnchorLocalPosition(), readDistanceFromAnchor());

   // claimed right here rather than at the end of the frame, so climbing can read its own keys on the
   // very frame the rope is grabbed instead of sitting at zero for one frame
   _hold.syncVerticalKeyClaim(input._controls, true);
}

float PlayerRope::readDistanceAlongRope() const
{
   // the rope between the suspension point and the player is the segments above him plus the link
   return (static_cast<float>(_element_index) * _segment_length_m) + _link_length_m;
}

float PlayerRope::readDistanceFromAnchor() const
{
   return readDistanceAlongRope() + anchor_limit_slack_m;
}

void PlayerRope::updateHold(const sf::Time& dt, const RopeInput& input)
{
   if (!_rope || !input._player_body)
   {
      reset();
      return;
   }

   updateClimb(dt, input);

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

void PlayerRope::updateClimb(const sf::Time& dt, const RopeInput& input)
{
   // up and down cancel each other out
   const auto up_pressed = _hold.isUpPressed();
   const auto down_pressed = _hold.isDownPressed();

   if (up_pressed == down_pressed)
   {
      return;
   }

   const auto& chain_elements = _rope->getChainElements();

   if (chain_elements.empty())
   {
      reset();
      return;
   }

   const auto climbing_up = up_pressed;
   const auto last_index = static_cast<int32_t>(chain_elements.size()) - 1;

   // shortening the link alone does not lift the player: the chain carries a body several times the
   // mass of one link, so box2d pulls the light rope down instead of hoisting him. the pull towards
   // the element above is what turns climbing up into actual climbing.
   if (climbing_up)
   {
      const auto target_index = std::max(_element_index - 1, 0);
      _hold.pullPlayerTowards(
         input._player_body, chain_elements[static_cast<size_t>(target_index)]->GetPosition(), climb_pull_acceleration
      );
   }

   if (climbing_up && readDistanceAlongRope() <= climb_min_distance_from_anchor_m)
   {
      return;
   }

   const auto climb_speed_mps = climbing_up ? -climb_speed_up_mps : climb_speed_down_mps;
   _link_length_m += climb_speed_mps * dt.asSeconds();

   // the link only ever spans one segment; crossing that boundary moves the hold to the neighbouring
   // element, which is what walks the player along a chain that stays as it is
   auto element_index = _element_index;

   while (_link_length_m < 0.0f && element_index > 0)
   {
      element_index--;
      _link_length_m += _segment_length_m;
   }

   while (_link_length_m > _segment_length_m && element_index < last_index)
   {
      element_index++;
      _link_length_m -= _segment_length_m;
   }

   // the top of the rope and the free end below the last element are where climbing stops
   _link_length_m = std::clamp(_link_length_m, 0.0f, _segment_length_m);

   if (element_index != _element_index)
   {
      _element_index = element_index;
      bindToElement();
   }
   else
   {
      _hold.setLinkLength(_link_length_m);
   }

   _hold.setAnchorLimitDistance(readDistanceFromAnchor());
}

void PlayerRope::bindToElement()
{
   auto* element = readHeldElement();

   if (!element)
   {
      reset();
      return;
   }

   _hold.rebind(element, b2Vec2{0.0f, 0.0f}, _link_length_m);
}

b2Body* PlayerRope::readHeldElement() const
{
   if (!_rope)
   {
      return nullptr;
   }

   const auto& chain_elements = _rope->getChainElements();

   if (_element_index < 0 || _element_index >= static_cast<int32_t>(chain_elements.size()))
   {
      return nullptr;
   }

   return chain_elements[static_cast<size_t>(_element_index)];
}

void PlayerRope::release()
{
   _hold.detach();
   _rope.reset();
   _element_index = 0;
   _link_length_m = 0.0f;
   _segment_length_m = 0.0f;
   _release_grace_remaining_s = release_grace_duration_s;
   _regrab_blocked_s = regrab_block_duration_s;
}

void PlayerRope::reset()
{
   _hold.detach();
   _hold.syncVerticalKeyClaim({}, false);
   _rope.reset();
   _element_index = 0;
   _link_length_m = 0.0f;
   _segment_length_m = 0.0f;
   _release_grace_remaining_s = 0.0f;
   _regrab_blocked_s = 0.0f;
   _jump_button_was_pressed = false;
}

bool PlayerRope::isAttached() const
{
   return _hold.isAttached();
}

bool PlayerRope::isReleaseGraceActive() const
{
   return _release_grace_remaining_s > 0.0f;
}
