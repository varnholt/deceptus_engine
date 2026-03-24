#pragma once

#include "box2d/box2d.h"

#include "SFML/Graphics.hpp"

#include <array>
#include <unordered_set>
#include <vector>

#include "game/level/luanode.h"

namespace WorldQuery
{

/// \brief box2d query callback that collects every fixture overlapping a queried aabb.
class FixtureQueryCallback : public b2QueryCallback
{
public:
   /// \brief stores a reported fixture and continues the query.
   /// \param fixture fixture reported by box2d.
   /// \return true to continue iterating through remaining fixtures.
   bool ReportFixture(b2Fixture* fixture);
   std::vector<b2Fixture*> _fixtures;
};

/// \brief box2d query callback that collects bodies while skipping an optional ignore list.
class BodyQueryCallback : public b2QueryCallback
{
public:
   /// \brief creates a body-query callback with an ignore list.
   /// \param ignore_list bodies that should not be added to the query result.
   explicit BodyQueryCallback(const std::unordered_set<b2Body*>& ignore_list = {});

   /// \brief stores the fixture body unless it is present in the ignore list.
   /// \param fixture fixture reported by box2d.
   /// \return true to continue iterating through remaining fixtures.
   bool ReportFixture(b2Fixture* fixture) override;
   std::vector<b2Body*> _bodies;

private:
   const std::unordered_set<b2Body*>& _ignore_list;
};

/// \brief collision result entry containing a Lua node and the intersecting translated hitbox.
struct CollidedNode
{
   std::shared_ptr<LuaNode> _node;
   sf::FloatRect _hitbox;
};

/// \brief quadtree-like spatial node that stores bodies in a rectangle and optional child subdivisions.
struct OctreeNode
{
   sf::FloatRect _boundaries;
   std::vector<b2Body*> _bodies;
   std::array<std::unique_ptr<OctreeNode>, 4> _children;
   bool _is_leaf = true;

   /// \brief creates a spatial node, queries bodies in bounds, and recursively subdivides when needed.
   /// \param bounds world-space rectangle covered by this node.
   /// \param world physics world queried for bodies.
   /// \param depth current recursion depth.
   /// \param max_depth maximum subdivision depth.
   /// \param ignore_list bodies excluded from queries.
   OctreeNode(
      const sf::FloatRect& bounds,
      const std::shared_ptr<b2World>& world,
      int32_t depth = 0,
      int32_t max_depth = 3,
      const std::unordered_set<b2Body*>& ignore_list = {}
   );

   /// \brief prints body counts per leaf cell for a fixed depth grid.
   /// \param max_depth depth used to format and print the leaf grid.
   void debugBodyCounts(int32_t max_depth) const;

   /// \brief collects rectangle bounds from leaf nodes up to the requested depth.
   /// \param depth current traversal depth.
   /// \param max_depth depth at which to collect leaf rectangles.
   /// \return list of leaf rectangles in sfml coordinate space.
   std::vector<sf::FloatRect> collectLeafBounds(int32_t depth, int32_t max_depth) const;

private:
   /// \brief accumulates body counts into a flattened leaf grid index.
   /// \param leaf_body_counts output counters per leaf grid cell.
   /// \param depth current traversal depth.
   /// \param max_depth depth where counts are collected.
   /// \param index flattened index offset for this subtree.
   void countBodiesInLeaves(std::vector<int32_t>& leaf_body_counts, int32_t depth, int32_t max_depth, int32_t index) const;

   /// \brief splits this node into four children and queries each child region.
   /// \param world physics world queried for child nodes.
   /// \param depth depth used for child node construction.
   /// \param max_depth maximum allowed subdivision depth.
   void subdivide(const std::shared_ptr<b2World>& world, int32_t depth, int32_t max_depth);
   const std::unordered_set<b2Body*>& _ignore_list;
};

/// \brief queries fixtures overlapping an aabb.
/// \param world physics world to query.
/// \param aabb axis-aligned bounding box in box2d meters.
/// \return all fixtures reported by box2d for the requested aabb.
std::vector<b2Fixture*> queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb);

/// \brief queries bodies overlapping an aabb while excluding ignored bodies.
/// \param world physics world to query.
/// \param aabb axis-aligned bounding box in box2d meters.
/// \param ignore_list bodies to skip in the result.
/// \return bodies reported by the query callback, excluding ignored ones.
std::vector<b2Body*>
queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb, const std::unordered_set<b2Body*>& ignore_list = {});

/// \brief retrieves bodies whose fixtures overlap a screen-space rectangle.
/// \param world physics world to query.
/// \param screen rectangle in sfml coordinates.
/// \param ignore_list bodies to skip in the result.
/// \return bodies overlapping the converted rectangle.
std::vector<b2Body*> retrieveBodiesInsideRect(
   const std::shared_ptr<b2World>& world,
   const sf::FloatRect& screen,
   const std::unordered_set<b2Body*>& ignore_list = {}
);

/// \brief retrieves bodies in a rectangle that contain at least one enemy fixture.
/// \param world physics world to query.
/// \param rect rectangle in sfml coordinates.
/// \param ignore_list bodies to skip before filtering.
/// \return bodies in the rectangle that have an ObjectTypeEnemy fixture.
std::vector<b2Body*> retrieveEnemyBodiesInsideRect(
   const std::shared_ptr<b2World>& world,
   const sf::FloatRect& rect,
   const std::unordered_set<b2Body*>& ignore_list
);

/// \brief retrieves bodies in a rectangle that contain at least one fixture of the requested types.
/// \param world physics world to query.
/// \param rect rectangle in sfml coordinates.
/// \param ignore_list bodies to skip before filtering.
/// \param types accepted fixture object types.
/// \return bodies in the rectangle that match at least one requested type.
std::vector<b2Body*> retrieveBodiesInsideRectOfTypes(
   const std::shared_ptr<b2World>& world,
   const sf::FloatRect& rect,
   const std::unordered_set<b2Body*>& ignore_list,
   const std::unordered_set<ObjectType>& types
);

/// \brief finds Lua nodes whose translated hitboxes intersect a single search rectangle.
/// \param attack_rect search rectangle in sfml coordinates.
/// \return collided nodes with the specific hitbox rectangle that intersected.
std::vector<WorldQuery::CollidedNode> findNodesByHitbox(const sf::FloatRect& attack_rect);

/// \brief finds Lua nodes whose translated hitboxes intersect any rectangle in a set.
/// \param attack_rects search rectangles in sfml coordinates.
/// \return collided nodes with the first intersecting hitbox rectangle per node.
std::vector<WorldQuery::CollidedNode> findNodesByHitbox(const std::vector<sf::FloatRect>& attack_rects);

}  // namespace WorldQuery
