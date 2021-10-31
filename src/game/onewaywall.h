#pragma once

#include <Box2D/Box2D.h>

namespace OneWayWall
{

void beginContact(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture);
void endContact(b2Contact* contact);
void drop();

};

