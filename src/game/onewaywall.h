#pragma once

#include <Box2D/Box2D.h>

#include <set>


struct OneWayWall
{
   static OneWayWall& instance();

   void beginContact(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture);
   void endContact(b2Contact* contact);
   void clear();
   void drop();
   bool hasContacts() const;

private:

   OneWayWall() = default;
   std::set<b2Contact*> _contacts;
};

