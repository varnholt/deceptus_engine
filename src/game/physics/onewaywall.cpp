#include "onewaywall.h"

#include "game/level/fixturenode.h"

#include <iostream>

OneWayWall& OneWayWall::instance()
{
   static OneWayWall _instance;
   return _instance;
}

void OneWayWall::beginContact(b2Contact* contact, b2Fixture* player_fixture)
{
   _contacts.insert(contact);

   // decide whether an incoming contact to the platform should be disabled or not

   // if the head bounces against the one-sided wall, disable the contact
   // until there is no more contact with the head (EndContact), regardless of the velocity
   if (player_fixture && player_fixture->GetUserData().pointer && (static_cast<FixtureNode*>(player_fixture->GetUserData().pointer))->hasFlag("head"))
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
