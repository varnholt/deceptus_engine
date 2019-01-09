#pragma once

#include <cstdint>
#include <vector>

struct b2Vec2;

namespace Mesh
{
   void weldVertices(b2Vec2* verts, int32_t count, float threshold = 0.3f);
   void join(b2Vec2* a, b2Vec2* b);

   void writeObj(const std::string& filename, const std::vector<b2Vec2>& vertices, const std::vector<std::vector<int32_t> >& faces);
}
