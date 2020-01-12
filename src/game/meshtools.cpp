#include "meshtools.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>


namespace
{
static const auto triangulate = false;
}


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


void Mesh::writeObj(
   const std::string& filename,
   const std::vector<b2Vec2>& vertices,
   const std::vector<std::vector<uint32_t>>& faces
)
{
   std::ofstream out(filename);

   out.setf(std::ios::fixed);
   for (const auto& v : vertices)
   {
      out << std::setprecision(3) << "v " << v.x << " " << v.y << " " << 0.0f << std::endl;
   }

   out << std::endl;

   for (const auto& face : faces)
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


void Mesh::readObj(
   const std::string& filename,
   std::vector<b2Vec2>& points,
   std::vector<std::vector<uint32_t>>& faces
)
{
   std::vector<Mesh::Vertex> vertices;
   std::vector<b2Vec2> normals;
   std::vector<b2Vec2> uvs;

   auto trimString = [](std::string& str) {
      const char* whiteSpace = " \t\n\r";
      size_t location = str.find_first_not_of(whiteSpace);
      str.erase(0, location);
      location = str.find_last_not_of(whiteSpace);
      str.erase(location + 1);
   };

   auto faceCount = 0u;

   std::ifstream objStream(filename, std::ios::in);

   if (!objStream)
   {
      std::cerr << "unable to open file: " << filename << std::endl;
      return;
   }

   std::string line, token;

   getline(objStream, line);

   while (!objStream.eof())
   {
     trimString(line);

     if (line.length( ) > 0 && line.at(0) != '#')
     {
         std::istringstream lineStream(line);

         lineStream >> token;

         if (token == "v")
         {
             float x = 0.0f;
             float y = 0.0f;
             float z = 0.0f;

             lineStream >> x >> y >> z;
             points.push_back(b2Vec2(x, y));
         }
         else if (token == "vt")
         {
             float s = 0.0f;
             float t = 0.0f;

             lineStream >> s >> t;
             uvs.push_back(b2Vec2(s, t));

         } else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            lineStream >> x >> y >> z;
            normals.push_back(b2Vec2(x, y));
         }
         else if (token == "f")
         {
            faceCount++;

            std::vector<uint32_t> face;

            size_t slash1 = 0;
            size_t slash2 = 0;

            while (lineStream.good())
            {
               std::string vertString;
               lineStream >> vertString;

               auto pIndex = 0u;
               auto nIndex = 0u;
               auto tcIndex = 0u;

               slash1 = vertString.find("/");

               if (slash1 == std::string::npos)
               {
                  pIndex = static_cast<uint32_t>(atoi(vertString.c_str()) - 1);
               }
               else
               {
                  slash2 = vertString.find("/", slash1 + 1);
                  pIndex = static_cast<uint32_t>(atoi( vertString.substr(0,slash1).c_str() ) - 1);

                  if( slash2 > slash1 + 1 )
                  {
                     tcIndex = static_cast<uint32_t>(atoi(vertString.substr(slash1 + 1, slash2).c_str() ) - 1);
                  }

                  nIndex = static_cast<uint32_t>(atoi( vertString.substr(slash2 + 1,vertString.length()).c_str() ) - 1);
               }

               Mesh::Vertex vertex;
               vertex.pIndex = pIndex;
               vertex.nIndex = nIndex;
               vertex.tcIndex = tcIndex;

               face.push_back(pIndex);
               vertices.push_back(vertex);
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.

            std::vector<uint32_t> faceIndices;
            if (face.size() > 3 && triangulate)
            {
               auto v0 = face[0];
               auto v1 = face[1];
               auto v2 = face[2];

               Mesh::Vertex vt0 = vertices[0];
               Mesh::Vertex vt1 = vertices[1];
               Mesh::Vertex vt2 = vertices[2];

               faceIndices.push_back(v0);
               faceIndices.push_back(v1);
               faceIndices.push_back(v2);

               vertices.push_back(vt0);
               vertices.push_back(vt1);
               vertices.push_back(vt2);

               for (auto i = 3u; i < face.size(); i++)
               {
                  v1 = v2;
                  v2 = face[i];

                  vt1 = vt2;
                  vt2 = vertices[i];

                  faceIndices.push_back(v0);
                  faceIndices.push_back(v1);
                  faceIndices.push_back(v2);

                  vertices.push_back(vt0);
                  vertices.push_back(vt1);
                  vertices.push_back(vt2);
               }
            }
            else
            {
               for (auto i = 0u; i < face.size(); i++)
               {
                  faceIndices.push_back(face[i]);
                  vertices.push_back(vertices[i]);
               }
            }

            faces.push_back(faceIndices);
         }
      }

      getline(objStream, line);
   }

   objStream.close();

   std::cout << "Loaded mesh from: " << filename << std::endl;

   std::cout << " " << points.size()     << " points"      << std::endl;
   std::cout << " " << faceCount         << " faces"       << std::endl;
   std::cout << " " << faces.size() / 3  << " triangles."  << std::endl;
   std::cout << " " << normals.size()    << " normals"     << std::endl;
   std::cout << " " << uvs.size()        << " uvs"         << std::endl;
}

