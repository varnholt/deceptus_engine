#include "chainshapeanalyzer.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <SFML/Graphics.hpp>

#include "constants.h"
#include "fixturenode.h"
#include "player/player.h"

namespace
{

std::set<std::pair<float, float>> _conflicting_pos_set_m;
std::vector<b2Vec2> _conflicting_positions_m;
std::vector<sf::Vector2f> _conflicting_positions_px;

struct IndexedVector
{
   int32_t _chain_index{0};
   int32_t _vertex_index{0};
   std::pair<float, float> _pos;
   ObjectType _object_type{ObjectType::ObjectTypeInvalid};
   b2ChainShape* _chain{nullptr};

   bool operator<(const IndexedVector& v) const
   {
      return _pos < v._pos;
   }
};

}  // namespace


void ChainShapeAnalyzer::analyze(const std::shared_ptr<b2World>& world)
{
   static std::set<IndexedVector> _indexed_vectors;
   _indexed_vectors.clear();

   int32_t chain_index = 0;
   for (auto body = world->GetBodyList(); body != nullptr; body = body->GetNext())
   {
      if (body->GetType() != b2_staticBody)
      {
         continue;
      }

      auto fixture = body->GetFixtureList();
      while (fixture)
      {
         int32_t vertex_index = 0;

         auto next = fixture->GetNext();
         auto shape = fixture->GetShape();

         if (shape->GetType() != b2Shape::e_chain)
         {
            fixture = next;
            continue;
         }

         chain_index++;

         auto chain = dynamic_cast<b2ChainShape*>(shape);

         auto object_type = ObjectType::ObjectTypeInvalid;
         auto user_data = static_cast<FixtureNode*>(fixture->GetUserData());
         if (user_data)
         {
            object_type = user_data->getType();
         }

         for (auto i = 0; i < chain->m_count; i++)
         {
            auto pos = chain->m_vertices[i];

            const auto iv = IndexedVector{chain_index, vertex_index, {pos.x, pos.y}, object_type, chain};
            const auto it = _indexed_vectors.find(iv);

            if (it == _indexed_vectors.end())
            {
               _indexed_vectors.insert(iv);
            }
            else if (chain_index != it->_chain_index && object_type != it->_object_type)
            {
               // check if the vertex needs a ghost vertex on the left or on the right
               auto isOnTheRightOfCollisionPos = [](const b2Vec2& colliding_vertex, b2Vec2* vertices, int32_t count)
               {
                  return std::any_of(
                     vertices,
                     vertices + count,
                     [colliding_vertex](auto pos)
                     {
                        // std::cout << pos.x * PPM << " vs " << colliding_vertex.x * PPM << std::endl;
                        return (pos.x - 0.001f > colliding_vertex.x) && (fabs(pos.y - colliding_vertex.y) < 0.001f);
                     }
                  );
               };

               const auto chain_is_on_the_right_of_the_collision =
                  isOnTheRightOfCollisionPos(pos, chain->m_vertices, chain->m_count);

               // std::cout << chain_is_on_the_right_of_the_collision << std::endl;

               const auto collision_is_on_the_right_of_the_one_way_wall =
                  (object_type == ObjectTypeSolid && chain_is_on_the_right_of_the_collision);

               // clang-format off
               std::cout
                  << "chain " << chain_index
                  << ", vertex: [" << vertex_index << "/" << (chain->m_count - 1)
                  << "], pos(" << iv._pos.first * PPM << ", " << iv._pos.second * PPM
                  << ") collides with chain: " << it->_chain_index
                  << ", vertex: [" << it->_vertex_index << "/" << (it->_chain->m_count - 1) << "]"
                  << ", pos(" << it->_pos.first * PPM << ", " << it->_pos.second * PPM << ")"
                  << " on the " << (collision_is_on_the_right_of_the_one_way_wall ? "right" : "left") << " side"
                  << std::endl;
               // clang-format on

               _conflicting_pos_set_m.insert({pos.x, pos.y});
            }
            vertex_index++;
         }

         fixture = next;
      }
   }

   for (const auto& pos_m : _conflicting_pos_set_m)
   {
      _conflicting_positions_m.push_back({pos_m.first, pos_m.second});
      _conflicting_positions_px.push_back({pos_m.first * PPM, pos_m.second * PPM});
   }
}


bool ChainShapeAnalyzer::checkPlayerAtCollisionPosition()
{
   auto player = Player::getCurrent();
   auto foot_sensor_fixture = player->getFootSensorFixture();
   const auto& world_transform = player->getBody()->GetTransform();

   for (const auto& bad_pos_m : _conflicting_positions_m)
   {
      const auto point_inside_rect = foot_sensor_fixture->GetShape()->TestPoint(world_transform, bad_pos_m);
      if (point_inside_rect)
      {
         return true;
      }
   }

   return false;
}


// tile pos: 252, 115



/*

chain 40, vector: 0, pos(6048, 2784) collides with chain: 12, vector: 3, pos(6048, 2784) on the right side
chain 49, vector: 19, pos(9480, 2400) collides with chain: 16, vector: 0, pos(9480, 2400) on the left side
chain 49, vector: 20, pos(9360, 2400) collides with chain: 17, vector: 3, pos(9360, 2400) on the right side
chain 49, vector: 52, pos(6360, 2952) collides with chain: 9, vector: 3, pos(6360, 2952) on the right side
chain 53, vector: 47, pos(10584, 1968) collides with chain: 22, vector: 0, pos(10584, 1968) on the left side

chain 40, vector: 0, pos(6048, 2784) collides with chain: 12, vector: 3, pos(6048, 2784)
correct, bridge on the left, floor on the right
+---------++----------
|/////////||
+---------+|
           |
           |
           +----------


chain 49, vector: 19, pos(9480, 2400) collides with chain: 16, vector: 0, pos(9480, 2400)
----------++-----------+
          ||///////////|
          |+-----------+
          |
          |
----------+

chain 49, vector: 20, pos(9360, 2400) collides with chain: 17, vector: 3, pos(9360, 2400)
+---------++----------
|/////////||
+---------+|
           |
           |
           +----------


chain 49, vector: 52, pos(6360, 2952) collides with chain: 9, vector: 3, pos(6360, 2952)
+---------++----------
|/////////||
+---------+|
           |
           |
           +----------



chain 53, vector: 47, pos(10584, 1968) collides with chain: 22, vector: 0, pos(10584, 1968)
----------++-----------+
          ||///////////|
          |+-----------+
          |
          |
----------+
*/


