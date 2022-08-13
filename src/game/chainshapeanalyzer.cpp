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
            auto v = chain->m_vertices[i];

            const auto iv = IndexedVector{chain_index, vector_index, {v.x, v.y}, object_type};
            const auto it = _indexed_vectors.find(iv);

            if (it == _indexed_vectors.end())
            {
               _indexed_vectors.insert(iv);
            }
            else if (chain_index != it->_chain_index && object_type != it->_object_type)
            {
               std::cout << "chain " << chain_index << ", vector: " << vector_index << ", pos(" << iv._pos.first * PPM << ", "
                         << iv._pos.second * PPM << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vector_index
                         << ", pos(" << it->_pos.first * PPM << ", " << it->_pos.second * PPM << ")" << std::endl;
            }
            vector_index++;
         }

         fixture = next;
      }
   }
}
