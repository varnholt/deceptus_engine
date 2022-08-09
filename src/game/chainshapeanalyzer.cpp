#include "chainshapeanalyzer.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace
{

constexpr auto epsilon = 0.001f;

struct IndexedVector
{
   int32_t _chain_index{0};
   int32_t _vector_index{0};
   b2Vec2 _pos;

   bool operator==(const IndexedVector& v) const
   {
      return (fabs(_pos.x - v._pos.x) < epsilon && fabs(_pos.y - v._pos.y) < epsilon);
   }
};

struct HashFunction
{
   size_t operator()(const IndexedVector& v) const
   {
      const auto hash_x = std::hash<float>()(v._pos.x);
      const auto hash_y = std::hash<float>()(v._pos.y) << 1;
      return hash_x ^ hash_y;
   }
};

}  // namespace

void ChainShapeAnalyzer::analyze(std::vector<std::vector<b2Vec2>>& chains)
{
   static std::unordered_set<IndexedVector, HashFunction> _indexed_vectors;

   int32_t chain_index = 0;
   for (const auto& chain : chains)
   {
      int32_t vector_index = 0;
      std::for_each(
         chain.begin(),
         chain.end(),
         [chain_index, &vector_index](const auto& v)
         {
            const auto iv = IndexedVector{chain_index, vector_index, v};
            const auto it = _indexed_vectors.find(iv);

            if (it == _indexed_vectors.end())
            {
               _indexed_vectors.insert(iv);
            }
            else
            {
               std::cout << "chain " << chain_index << "vector: " << vector_index << ", pos(" << iv._pos.x << ", " << iv._pos.y
                         << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vector_index << std::endl;
            }
            vector_index++;
         }
      );

      chain_index++;
   }
}
