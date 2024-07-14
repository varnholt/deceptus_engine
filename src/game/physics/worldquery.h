#pragma once

#include <box2d/box2d.h>

#include "SFML/Graphics.hpp"

#include <vector>

#include "game/level/luanode.h"

namespace WorldQuery
{

class FixtureQueryCallback : public b2QueryCallback
{
   public:

      bool ReportFixture(b2Fixture* fixture);
      std::vector<b2Fixture*> _fixtures;
};


class BodyQueryCallback : public b2QueryCallback
{
   public:

      bool ReportFixture(b2Fixture* fixture);
      std::vector<b2Body*> _bodies;
};


std::vector<b2Fixture*> queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb);
std::vector<b2Body*> queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb);
std::vector<b2Body*> retrieveBodiesOnScreen(const std::shared_ptr<b2World>& world, const sf::FloatRect& screen);
std::vector<std::shared_ptr<LuaNode>> findNodes(const sf::FloatRect& attack_rect);
std::vector<std::shared_ptr<LuaNode>> findNodes(const std::vector<sf::FloatRect>& attack_rects);

}

