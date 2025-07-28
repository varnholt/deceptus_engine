#include "game/physics/worldquery.h"

#include <iostream>
#include <memory>
#include <ranges>
#include <vector>

#include "game/level/fixturenode.h"
#include "game/level/luainterface.h"

b2Vec2 vecS2B(const sf::Vector2f& vector)
{
   return {vector.x * MPP, vector.y * MPP};
}

std::vector<b2Fixture*> WorldQuery::queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb)
{
   FixtureQueryCallback query_callback;
   world->QueryAABB(&query_callback, aabb);
   return query_callback._fixtures;
}

bool WorldQuery::FixtureQueryCallback::ReportFixture(b2Fixture* fixture)
{
   _fixtures.push_back(fixture);
   return true;
}

std::vector<b2Body*>
WorldQuery::queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb, const std::unordered_set<b2Body*>& ignore_list)
{
   BodyQueryCallback query_callback(ignore_list);
   world->QueryAABB(&query_callback, aabb);
   return query_callback._bodies;
}

WorldQuery::BodyQueryCallback::BodyQueryCallback(const std::unordered_set<b2Body*>& ignore_list) : _ignore_list(ignore_list)
{
}

bool WorldQuery::BodyQueryCallback::ReportFixture(b2Fixture* fixture)
{
   auto* body = fixture->GetBody();
   if (!_ignore_list.contains(body))
   {
      _bodies.push_back(body);
   }

   return true;
}

std::vector<WorldQuery::CollidedNode> WorldQuery::findNodesByHitbox(const sf::FloatRect& search_rect)
{
   const auto& nodes = LuaInterface::instance().getObjectList();

   std::vector<WorldQuery::CollidedNode> hit_nodes;

   for (const auto& node : nodes)
   {
      if (auto intersecting_hitbox = std::ranges::find_if(
             node->_hitboxes,
             [&search_rect](const auto& hit_box) { return hit_box.getRectTranslated().findIntersection(search_rect).has_value(); }
          );
          intersecting_hitbox != node->_hitboxes.end())
      {
         hit_nodes.emplace_back(WorldQuery::CollidedNode{node, intersecting_hitbox->getRectTranslated()});
      }
   }

   return hit_nodes;
}

std::vector<WorldQuery::CollidedNode> WorldQuery::findNodesByHitbox(const std::vector<sf::FloatRect>& attack_rects)
{
   const auto& nodes = LuaInterface::instance().getObjectList();

   std::vector<WorldQuery::CollidedNode> hit_nodes;

   for (const auto& node : nodes)
   {
      if (auto intersecting_hitbox = std::ranges::find_if(
             node->_hitboxes,
             [&attack_rects](const auto& hit_box)
             {
                return std::ranges::any_of(
                   attack_rects,
                   [&hit_box](const auto& attack_rect) { return hit_box.getRectTranslated().findIntersection(attack_rect).has_value(); }
                );
             }
          );
          intersecting_hitbox != node->_hitboxes.end())
      {
         hit_nodes.emplace_back(WorldQuery::CollidedNode{node, intersecting_hitbox->getRectTranslated()});
      }
   }

   return hit_nodes;
}

std::vector<b2Body*> WorldQuery::retrieveBodiesInsideRect(
   const std::shared_ptr<b2World>& world,
   const sf::FloatRect& rect,
   const std::unordered_set<b2Body*>& ignore_list
)
{
   b2AABB aabb;

   const auto l = rect.position.x;
   const auto r = rect.position.x + rect.size.x;
   const auto t = rect.position.y;
   const auto b = rect.position.y + rect.size.y;

   aabb.upperBound = vecS2B({std::max(l, r), std::max(b, t)});
   aabb.lowerBound = vecS2B({std::min(l, r), std::min(b, t)});

   return WorldQuery::queryBodies(world, aabb, ignore_list);
}

std::vector<b2Body*> WorldQuery::retrieveEnemyBodiesInsideRect(
   const std::shared_ptr<b2World>& world,
   const sf::FloatRect& rect,
   const std::unordered_set<b2Body*>& ignore_list
)
{
   // retrieve all bodies inside the rectangle
   auto all_bodies = WorldQuery::retrieveBodiesInsideRect(world, rect, ignore_list);

   // filter bodies of type ObjectTypeEnemy
   auto filtered_bodies = all_bodies | std::views::filter(
                                          [](b2Body* body)
                                          {
                                             for (auto fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
                                             {
                                                auto user_data = fixture->GetUserData().pointer;
                                                if (!user_data)
                                                {
                                                   continue;
                                                }

                                                auto fixture_node = static_cast<FixtureNode*>(user_data);
                                                if (fixture_node->getType() == ObjectTypeEnemy)
                                                {
                                                   return true;  // found at least one enemy fixture
                                                }
                                             }
                                             return false;  // no enemy fixtures found
                                          }
                                       );

   // convert the view to a vector
   return std::vector<b2Body*>{filtered_bodies.begin(), filtered_bodies.end()};
}

WorldQuery::OctreeNode::OctreeNode(
   const sf::FloatRect& bounds,
   const std::shared_ptr<b2World>& world,
   int32_t depth,
   int32_t max_depth,
   const std::unordered_set<b2Body*>& ignore_list
)
    : _boundaries(bounds), _ignore_list(ignore_list)
{
   _bodies = WorldQuery::retrieveBodiesInsideRect(world, bounds, ignore_list);

   if (!_bodies.empty() && depth < max_depth)
   {
      _is_leaf = false;
      subdivide(world, depth + 1, max_depth);
   }
}

void WorldQuery::OctreeNode::countBodiesInLeaves(std::vector<int32_t>& leaf_body_counts, int32_t depth, int32_t max_depth, int32_t index)
   const
{
   if (depth == max_depth)
   {
      if (_is_leaf && !_bodies.empty())
      {
         leaf_body_counts[index] += _bodies.size();
      }
   }
   else
   {
      // calculate the offset for the child indices at the next depth level
      const int32_t child_offset = 1 << (2 * (max_depth - depth - 1));
      for (int32_t i = 0; i < 4; ++i)
      {
         if (_children[i])
         {
            _children[i]->countBodiesInLeaves(leaf_body_counts, depth + 1, max_depth, index + i * child_offset);
         }
      }
   }
}

void WorldQuery::OctreeNode::debugBodyCounts(int32_t max_depth) const
{
   const int32_t leaf_count = 1 << (2 * max_depth);  // equivalent to 4 ^ max_depth
   std::vector<int32_t> leaf_body_counts(leaf_count, 0);

   countBodiesInLeaves(leaf_body_counts, max_depth, 0, 0);

   const int32_t grid_size = 1 << max_depth;  // 2 ^ max_depth
   for (int32_t y = 0; y < grid_size; ++y)
   {
      for (int32_t x = 0; x < grid_size; ++x)
      {
         std::cout << leaf_body_counts[y * grid_size + x] << " ";
      }
      std::cout << "\n";
   }
}

std::vector<sf::FloatRect> WorldQuery::OctreeNode::collectLeafBounds(int32_t depth, int32_t max_depth) const
{
   std::vector<sf::FloatRect> leaf_bounds;

   if (depth == max_depth)
   {
      // at max depth, add the bounds of this leaf node
      if (_is_leaf)
      {
         leaf_bounds.push_back(_boundaries);
      }
   }
   else
   {
      // recurse into each child node and collect their bounds
      for (const auto& child : _children)
      {
         if (child)
         {
            // collect bounds from child and append to the main vector
            const auto child_boundaries = child->collectLeafBounds(depth + 1, max_depth);
            leaf_bounds.insert(leaf_bounds.end(), child_boundaries.begin(), child_boundaries.end());
         }
      }
   }

   return leaf_bounds;
}

void WorldQuery::OctreeNode::subdivide(const std::shared_ptr<b2World>& world, int32_t depth, int32_t max_depth)
{
   // use half sizes
   const auto half_width = _boundaries.size.x / 2.0f;
   const auto half_height = _boundaries.size.y / 2.0f;

   std::array<sf::Vector2f, 4> offsets = {
      sf::Vector2f(0, 0),                    // top-left
      sf::Vector2f(half_width, 0),           // top-right
      sf::Vector2f(0, half_height),          // bottom-left
      sf::Vector2f(half_width, half_height)  // bottom-right
   };

   for (int32_t i = 0; i < 4; ++i)
   {
      sf::FloatRect child_bounds(
         {_boundaries.position.x + offsets[i].x,   // x-coordinate of top-left corner
          _boundaries.position.y + offsets[i].y},  // y-coordinate of top-left corner
         {half_width,                              // width of the child rectangle
          half_height}                             // height of the child rectangle
      );

      _children[i] = std::make_unique<OctreeNode>(child_bounds, world, depth, max_depth, _ignore_list);
   }
}
