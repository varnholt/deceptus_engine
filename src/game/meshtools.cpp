#include "meshtools.h"

#include "Box2D/Box2D.h"

#include <fstream>
#include <iomanip>
#include <ostream>


void Mesh::weldVertices(b2Vec2* verts, int32_t count, float threshold)
{
   for (auto i = 0; i < count; i++)
   {
      for (auto j = 0; j < count; j++)
      {
         const auto& vi = verts[i];
         const auto& vj = verts[j];

         const auto dist = b2Distance(vi, vj);

         if (dist < threshold)
         {
            // join(vi, vj);
         }
      }
   }
}

// "layer_" + layer->mName + ".obj"

void Mesh::writeObj(
   const std::string& filename,
   const std::vector<b2Vec2>& vertices,
   const std::vector<std::vector<int32_t>>& faces
)
{
   std::ofstream out(filename);

   out.setf(std::ios::fixed);
   for (const auto v : vertices)
   {
      out << std::setprecision(3) << "v " << v.x << " " << v.y << " " << 0.0f << std::endl;
   }

   out << std::endl;

   for (const auto face : faces)
   {
      out << "f ";
      for (const auto p : face)
      {
         out << p << " ";
      }
      out << std::endl;
   }

   out.close();

}

void Mesh::join(b2Vec2* a, b2Vec2* b)
{
   a = b;
}


/*

void WeldVerticesSlow(vertex *verts, int vertCount)
{
    for (int a=0;a<vertCount;a++)
    {
        for (int b=0;b<vertCount;b++)
        {
            if (Distance(verts[a], verts[b]) < THRESHOLD)
                Join(a, b);
        }
    }
}


Now I'm not claiming that this one below is the fastest algorithm, but it's kind of somewhere around O(n-log-n) I think.

void WeldVerticesFast(vertex *verts, int vertCount)
{
    SortAlongXAxis(verts, vertCount);

    for (int a=0;a<vertCount;a++)
    {
        int b = a;
        while(b--)
        {
            if (verts[b].x < verts[a].x - THRESHOLD)
                break;

            if (Distance(verts[a], verts[b]) < THRESHOLD)
                Join(a, b);
        }
    }
}


*/
