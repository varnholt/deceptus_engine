#include "chainshapeanalyzer.h"

#include <algorithm>
#include <iostream>
#include <set>

namespace
{

struct IndexedVector
{
   int32_t _chain_index{0};
   int32_t _vector_index{0};
   std::pair<float, float> _pos;

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
            const auto iv = IndexedVector{chain_index, vector_index, {v.x, v.y}};
            const auto it = _indexed_vectors.find(iv);

            if (it == _indexed_vectors.end())
            {
               _indexed_vectors.insert(iv);
            }
            else if (chain_index != it->_chain_index)
            {
               std::cout << "chain " << chain_index << ", vector: " << vector_index << ", pos(" << iv._pos.first << ", " << iv._pos.second
                         << ") collides with chain: " << it->_chain_index << ", vector: " << it->_vector_index << ", pos(" << it->_pos.first
                         << ", " << it->_pos.second << ")" << std::endl;
            }
            vector_index++;
         }
      );

      chain_index++;
   }
}
