#pragma once

#include <box2d/box2d.h>

#include "SFML/Graphics.hpp"

#include <array>
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

struct CollidedNode
{
   std::shared_ptr<LuaNode> _node;
   sf::FloatRect _hitbox;
};

struct OctreeNode
{
   sf::FloatRect _boundaries;
   std::vector<b2Body*> _bodies;
   std::array<std::unique_ptr<OctreeNode>, 4> _children;
   bool _is_leaf = true;

   OctreeNode(const sf::FloatRect& bounds, const std::shared_ptr<b2World>& world, int32_t depth = 0, int32_t max_depth = 3);

   void debugBodyCounts(int32_t max_depth) const;

private:
   void countBodiesInLeaves(std::vector<int32_t>& leaf_body_counts, int32_t max_depth, int32_t depth, int32_t index) const;
   void subdivide(const std::shared_ptr<b2World>& world, int32_t depth, int32_t max_depth);
};

std::vector<b2Fixture*> queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb);
std::vector<b2Body*> queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb);
std::vector<b2Body*> retrieveBodiesInsideRect(const std::shared_ptr<b2World>& world, const sf::FloatRect& screen);
std::vector<WorldQuery::CollidedNode> findNodes(const sf::FloatRect& attack_rect);
std::vector<WorldQuery::CollidedNode> findNodes(const std::vector<sf::FloatRect>& attack_rects);

}  // namespace WorldQuery
