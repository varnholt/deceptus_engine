#pragma once

#include "box2d/box2d.h"

#include <set>

/// \brief tracks active one-way-wall contacts and toggles contact solidity based on player movement.
struct OneWayWall
{
   /// \brief returns the singleton one-way-wall handler.
   /// \return global one-way-wall state instance.
   static OneWayWall& instance();

   /// \brief handles begin contact and disables solidity while the player is moving upward.
   /// \param contact one-way-wall contact to track and potentially disable.
   /// \param player_fixture player fixture in this contact, used for head/velocity checks.
   void beginContact(b2Contact* contact, b2Fixture* player_fixture);
   /// \brief re-enables a contact and removes it from the tracked one-way-wall set.
   /// \param contact one-way-wall contact that just ended.
   void endContact(b2Contact* contact);
   /// \brief clears tracked contacts without changing their enabled state.
   void clear();
   /// \brief disables every tracked one-way-wall contact to force a drop-through.
   void drop();
   /// \brief reports whether any one-way-wall contacts are currently tracked.
   /// \return true when at least one one-way-wall contact is active.
   bool hasContacts() const;

private:
   OneWayWall() = default;
   std::set<b2Contact*> _contacts;
};
