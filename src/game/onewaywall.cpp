#include "onewaywall.h"

#include "fixturenode.h"

#include <iostream>


// the concept is simple:
//    when the player moves upwards into a one way wall, the contact is disabled
//    when the player moves downwards into a one way wall, the contact is NOT disabled
//
// to make this work, box2d code has been modified so that contacts that have been
// disabled once will remain disabled. see code change below:
//
// void b2Contact::Update(b2ContactListener* listener)
// {
//    b2Manifold oldManifold = m_manifold;
//
//    // Re-enable this contact.
//    // m_flags |= e_enabledFlag;


OneWayWall& OneWayWall::instance()
{
   static OneWayWall _instance;
   return _instance;
}


void OneWayWall::beginContact(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture)
{
   _contacts.insert(contact);

   // decide whether an incoming contact to the platform should be disabled or not

   // if the head bounces against the one-sided wall, disable the contact
   // until there is no more contact with the head (EndContact), regardless of the velocity
   if (player_fixture && (static_cast<FixtureNode*>(player_fixture->GetUserData()))->hasFlag("head"))
   {
      contact->SetEnabled(false);
   }

   // when moving down, the contact should be enabled
   if (player_fixture->GetBody()->GetLinearVelocity().y > 0.0f)
   {
      return;
   }

   // while going up, the contact should not be solid
   contact->SetEnabled(false);
}


void OneWayWall::endContact(b2Contact* contact)
{
   // reset the default state of the contact
   contact->SetEnabled(true);
   _contacts.erase(contact);
}


void OneWayWall::drop()
{
   std::cout << "disable " << _contacts.size() << " contacts" << std::endl;
   for (auto contact : _contacts)
   {
      contact->SetEnabled(false);
   }
}


bool OneWayWall::hasContacts() const
{
   return _contacts.size() > 0;
}


void OneWayWall::clear()
{
   _contacts.clear();
}
