#pragma once

#include <Box2D/Box2D.h>

namespace OneWayWall
{

void process(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture);

};

