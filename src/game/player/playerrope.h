#ifndef PLAYERROPE_H
#define PLAYERROPE_H

#include <SFML/Graphics.hpp>

#include "box2d/box2d.h"
#include "game/player/playerropehold.h"

#include <cstdint>
#include <memory>

class GrabRope;

/// \brief lets the player grab a rope mechanism in the level, climb it and swing on it.
/// \note this is the player side of the GrabRope mechanism. it reads the input, owns up and down while
///       the player hangs, and drives the shared rope hold - the mechanism itself never learns that
///       the player exists. the difference to the harpoon is that the chain here is already built: the
///       harpoon reels by adding and removing segments, this climbs by walking the hold along the
///       chain that the level provides.
class PlayerRope
{
public:
   /// \brief per-frame rope input gathered from player state and controls.
   struct RopeInput
   {
      std::shared_ptr<b2World> _world;
      std::shared_ptr<PlayerControls> _controls;
      b2Body* _player_body{nullptr};
      sf::FloatRect _player_rect_px;
      bool _jump_button_pressed{false};
      bool _move_left_pressed{false};
      bool _move_right_pressed{false};
      bool _in_air{false};
      bool _in_water{false};
      bool _dead{false};

      //!< the harpoon already carries the player, so a grab rope must not take him as well
      bool _carried_elsewhere{false};
   };

   /// \brief grabs, climbs, swings or lets go, depending on the current state and input.
   /// \param dt elapsed frame time.
   /// \param input rope command, environment state and player body.
   void update(const sf::Time& dt, const RopeInput& input);

   /// \brief lets go of the rope and returns to the idle state.
   void reset();

   /// \brief reports whether the player currently hangs on a rope.
   /// \return true while the player hangs.
   bool isAttached() const;

   /// \brief reports whether the player still keeps the momentum gained from a released swing.
   /// \return true while the post-release grace period is running.
   bool isReleaseGraceActive() const;

private:
   /// \brief looks for a rope to grab and hangs the player off its closest element.
   /// \param input current rope input.
   void updateGrab(const RopeInput& input);

   /// \brief climbs, swings and checks whether the player wants to let go.
   /// \param dt elapsed frame time.
   /// \param input current rope input.
   void updateHold(const sf::Time& dt, const RopeInput& input);

   /// \brief walks the hold up or down the chain while up or down is held.
   /// \param dt elapsed frame time.
   /// \param input current rope input.
   void updateClimb(const sf::Time& dt, const RopeInput& input);

   /// \brief hangs the player off the chain element closest to him.
   /// \param rope rope to grab.
   /// \param input current rope input.
   void grab(const std::shared_ptr<GrabRope>& rope, const RopeInput& input);

   /// \brief lets go of the rope and starts the momentum grace period.
   void release();

   /// \brief moves the hold onto the chain element the current link length points at.
   void bindToElement();

   /// \brief returns the chain element the player hangs on.
   /// \return chain element body, or nullptr when there is no rope.
   b2Body* readHeldElement() const;

   /// \brief returns how much rope there is between the suspension point and the player.
   /// \return length in box2d units.
   float readDistanceAlongRope() const;

   /// \brief returns the distance the anchor limit allows between the suspension point and the player.
   /// \return length in box2d units, including the slack the chain is allowed to sag by.
   float readDistanceFromAnchor() const;

   PlayerRopeHold _hold;
   std::shared_ptr<GrabRope> _rope;

   int32_t _element_index{0};      //!< index into the rope chain, 0 is the element at the anchor
   float _link_length_m{0.0f};     //!< distance below the held element, always within one segment
   float _segment_length_m{0.0f};  //!< spacing of the chain elements, cached from the rope
   float _regrab_blocked_s{0.0f};  //!< keeps letting go from grabbing the same rope again right away
   float _release_grace_remaining_s{0.0f};
   bool _jump_button_was_pressed{false};
};

#endif  // PLAYERROPE_H
