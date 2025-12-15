#pragma once

#include "box2d/box2d.h"

#include <set>

/*! \brief OneWayWall implements walls that are only solid from one side.
 *         This is the classic Mario style behavior where you jump through the bottom of the surface and
 *         as soon as the player is falling down again, the surface becomes solid.
 *
 *  The concept is simple:
 *     When the player moves upwards into a one way wall, the contact is disabled
 *     When the player moves downwards into a one way wall, the contact is NOT disabled
 *
 *  To make this work, box2d code has been modified so that contacts that have been
 *  disabled once will remain disabled. see code change below:
 *
 *  void b2Contact::Update(b2ContactListener* listener)
 *  {
 *     b2Manifold oldManifold = m_manifold;
 *
 *     // Re-enable this contact.
 *     // m_flags |= e_enabledFlag;
 */
struct OneWayWall
{
   static OneWayWall& instance();

   void beginContact(b2Contact* contact, b2Fixture* player_fixture);
   void endContact(b2Contact* contact);
   void clear();
   void drop();
   bool hasContacts() const;

private:
   OneWayWall() = default;
   std::set<b2Contact*> _contacts;
};
