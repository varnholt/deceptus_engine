#include "onewaywall.h"

#include "fixturenode.h"


void OneWayWall::process(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture)
{
   // decide whether an incoming contact to the platform should be disabled or not

   // if the head bounces against the one-sided wall, disable the contact
   // until there is no more contact with the head (EndContact)
   if (player_fixture && (static_cast<FixtureNode*>(player_fixture->GetUserData()))->hasFlag("head"))
   {
      contact->SetEnabled(false);
   }

   if (!platform_fixture)
   {
      return;
   }

   // if moving down, the contact should be solid
   if (player_fixture->GetBody()->GetLinearVelocity().y > 0.0f)
   {
      return;
   }

   // while going up, the contact should not be solid
   contact->SetEnabled(false);
}
