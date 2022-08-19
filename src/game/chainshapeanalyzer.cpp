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
   int32_t _vector_index{0};
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
                         << iv._pos.second * PPM << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vector_index
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
         int32_t vector_index = 0;

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

         for (auto i = 0; i < chain->GetChildCount(); i++)
         {
            auto pos = chain->m_vertices[i];

            const auto iv = IndexedVector{chain_index, vector_index, {pos.x, pos.y}, object_type, chain};
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
                  isOnTheRightOfCollisionPos(pos, chain->m_vertices, chain->GetChildCount());

               // std::cout << chain_is_on_the_right_of_the_collision << std::endl;

               const auto collision_is_on_the_right_of_the_one_way_wall =
                  (object_type == ObjectTypeSolid && chain_is_on_the_right_of_the_collision);

               std::cout << "chain " << chain_index << ", vector: " << vector_index << ", pos(" << iv._pos.first * PPM << ", "
                         << iv._pos.second * PPM << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vector_index
                         << ", pos(" << it->_pos.first * PPM << ", " << it->_pos.second * PPM << ")"
                         << " on the " << (collision_is_on_the_right_of_the_one_way_wall ? "right" : "left") << " side" << std::endl;
            }
            vector_index++;
         }

         fixture = next;
      }
   }
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


void ChainShapeAnalyzer::fix(b2ChainShape* chain, int32_t vertex_index)
{
//   auto vertex = chain->m_vertices[vertex_index];
//
// 1) find the right edge
//   void GetChildEdge(b2EdgeShape* edge, int32 index) const;
//
// 2) inject edge shape
// ground made of edges with ghost vertices set
//
//   myFixtureDef.shape = &edgeShape;
//   for (int i = 0; i < 21; i++) {
//      float left = -20+i*2 - 1;
//      float right = -20+i*2 + 1;
//      edgeShape.Set( b2Vec2(left,20+getHeight(left)), b2Vec2(right,20+getHeight(right)) );
//      edgeShape.m_vertex0.Set( left-1,20+getHeight(left) );
//      edgeShape.m_vertex3.Set( right+1,20+getHeight(right) );
//      edgeShape.m_hasVertex0 = true;
//      edgeShape.m_hasVertex3 = true;
//      fakeGroundBody->CreateFixture(&myFixtureDef);
//   }
//
//   b2EdgeShape edgeShape;
//   edgeShape.Set( v1, v2 );
//   edgeShape.m_vertex0.Set( v0 );
//   edgeShape.m_vertex3.Set( v3 );
//   edgeShape.m_hasVertex0 = true;
//   edgeShape.m_hasVertex3 = true;
}

