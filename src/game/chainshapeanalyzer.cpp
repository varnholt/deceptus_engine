#include "chainshapeanalyzer.h"

#include <algorithm>
#include <iostream>
#include <set>

#include "constants.h"
#include "fixturenode.h"

namespace
{

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

void ChainShapeAnalyzer::analyze(std::vector<std::vector<b2Vec2>>& chains)
{
   static std::set<IndexedVector> _indexed_vectors;
   _indexed_vectors.clear();

   int32_t chain_index = 0;
   for (const auto& chain : chains)
   {
      int32_t vector_index = 0;
      std::for_each(
         chain.begin(),
         chain.end(),
         [chain_index, &vector_index](const auto& v)
         {
            const auto iv = IndexedVector{chain_index, vector_index, {v.x, v.y}, ObjectType::ObjectTypeInvalid};
            const auto it = _indexed_vectors.find(iv);

            if (it == _indexed_vectors.end())
            {
               _indexed_vectors.insert(iv);
            }
            else if (chain_index != it->_chain_index)
            {
               std::cout << "chain " << chain_index << ", vector: " << vector_index << ", pos(" << iv._pos.first * PPM << ", "
                         << iv._pos.second * PPM << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vertex_index
                         << ", pos(" << it->_pos.first * PPM << ", " << it->_pos.second * PPM << ")" << std::endl;
            }
            vector_index++;
         }
      );

      chain_index++;
   }
}

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

               std::cout << "chain " << chain_index << ", vertex: [" << vertex_index << "/" << (chain->m_count - 1) << "], pos(" << iv._pos.first * PPM << ", "
                         << iv._pos.second * PPM << ") collides with chain: " << it->_chain_index << ", vertex: [" << it->_vertex_index << "/" << (it->_chain->m_count - 1) << "]"
                         << ", pos(" << it->_pos.first * PPM << ", " << it->_pos.second * PPM << ")"
                         << " on the " << (collision_is_on_the_right_of_the_one_way_wall ? "right" : "left") << " side" << std::endl;

               // the first and the last vertex are the same in a loop
               // if (vertex_index == 0 || vertex_index == chain->m_count - 1)
               {
                  // fix(chain, vertex_index, object_type, collision_is_on_the_right_of_the_one_way_wall);
               }

               // if (iv._vertex_index == 0 || iv._vertex_index == iv._chain->m_count - 1)
               {
                  // fix(iv._chain, iv._vertex_index, iv._object_type, collision_is_on_the_right_of_the_one_way_wall);
               }
            }
            vertex_index++;
         }

         fixture = next;
      }
   }
}

// idea: i know the positions where the player 'jumps' even though he shouldn't
// i could as well check the presolve code in box2d and mute impulses coming from there

// tile pos: 252, 115

void ChainShapeAnalyzer::fix(b2ChainShape* chain, int32_t vertex_index, ObjectType object_type, bool right)
{
//   auto conflicting_vertex = chain->m_vertices[vertex_index];
//   chain->m_nextVertex.y = conflicting_vertex.y;
//   chain->m_prevVertex.y = conflicting_vertex.y;

   if (vertex_index != 0 && vertex_index != chain->m_count -1)
   {
      // can't fix
      return;
   }

   const auto vertex = chain->m_vertices[vertex_index];
   b2Vec2 fixed_vertex{vertex};

   if (object_type == ObjectTypeSolidOneWay)
   {
      fixed_vertex.x += right ? 1.0f : -1.0f;
   }

   else if (object_type == ObjectTypeSolid)
   {
      // right is always seen from the perspective of the one way wall
      fixed_vertex.x += right ? -1.0f : 1.0f;
   }

   auto outputVertices = [](b2ChainShape* chain){
      std::cout
         << "pref: (" << chain->m_prevVertex.x << ", " << chain->m_prevVertex.y << "), ";
      for (auto i = 0; i < chain->m_count; i++)
      {
         std::cout
            << " (" << chain->m_vertices[i].x << ", " << chain->m_vertices[i].y << "), ";
      }
      std::cout
         << "next: (" << chain->m_nextVertex.x << ", " << chain->m_nextVertex.y << ") "
         << std::endl;
   };

   std::cout << "before: ";
   outputVertices(chain);

   if (vertex_index == 0)
   {
      chain->m_prevVertex = fixed_vertex;
   }
   else if (vertex_index == chain->m_count -1)
   {
      chain->m_nextVertex = fixed_vertex;
   }

   std::cout << "after:  ";
   outputVertices(chain);
}



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

