#include "meshtools.h"

#include "framework/tools/log.h"

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

void Mesh::writeObj(const std::string& filename, const std::vector<b2Vec2>& vertices, const std::vector<std::vector<uint32_t>>& faces)
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

void Mesh::readObj(const std::string& filename, std::vector<b2Vec2>& points, std::vector<std::vector<uint32_t>>& faces)
{
   std::vector<Mesh::Vertex> vertices;
   std::vector<b2Vec2> normals;
   std::vector<b2Vec2> uvs;

   auto trimString = [](std::string& str)
   {
      const char* whitespace = " \t\n\r";
      size_t location = str.find_first_not_of(whitespace);
      str.erase(0, location);
      location = str.find_last_not_of(whitespace);
      str.erase(location + 1);
   };

   auto faceCount = 0u;

   std::ifstream obj_stream(filename, std::ios::in);

   if (!obj_stream)
   {
      Log::Error() << "unable to open file: " << filename;
      return;
   }

   std::string line, token;

   getline(obj_stream, line);

   while (!obj_stream.eof())
   {
      trimString(line);

      if (line.length() > 0 && line.at(0) != '#')
      {
         std::istringstream line_stream(line);

         line_stream >> token;

         if (token == "v")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            line_stream >> x >> y >> z;
            points.push_back(b2Vec2(x, y));
         }
         else if (token == "vt")
         {
            float s = 0.0f;
            float t = 0.0f;

            line_stream >> s >> t;
            uvs.push_back(b2Vec2(s, t));
         }
         else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            line_stream >> x >> y >> z;
            normals.push_back(b2Vec2(x, y));
         }
         else if (token == "f")
         {
            faceCount++;

            std::vector<uint32_t> face;

            size_t slash_1 = 0;
            size_t slash_2 = 0;

            while (line_stream.good())
            {
               std::string vert_string;
               line_stream >> vert_string;

               auto p_index = 0u;
               auto nIndex = 0u;
               auto tcIndex = 0u;

               slash_1 = vert_string.find("/");

               if (slash_1 == std::string::npos)
               {
                  p_index = static_cast<uint32_t>(atoi(vert_string.c_str()) - 1);
               }
               else
               {
                  slash_2 = vert_string.find("/", slash_1 + 1);
                  p_index = static_cast<uint32_t>(atoi(vert_string.substr(0, slash_1).c_str()) - 1);

                  if (slash_2 > slash_1 + 1)
                  {
                     tcIndex = static_cast<uint32_t>(atoi(vert_string.substr(slash_1 + 1, slash_2).c_str()) - 1);
                  }

                  nIndex = static_cast<uint32_t>(atoi(vert_string.substr(slash_2 + 1, vert_string.length()).c_str()) - 1);
               }

               Mesh::Vertex vertex;
               vertex.pIndex = p_index;
               vertex.nIndex = nIndex;
               vertex.tcIndex = tcIndex;

               face.push_back(p_index);
               vertices.push_back(vertex);
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.

            std::vector<uint32_t> face_indices;
            if (face.size() > 3 && triangulate)
            {
               auto v0 = face[0];
               auto v1 = face[1];
               auto v2 = face[2];

               Mesh::Vertex vt0 = vertices[0];
               Mesh::Vertex vt1 = vertices[1];
               Mesh::Vertex vt2 = vertices[2];

               face_indices.push_back(v0);
               face_indices.push_back(v1);
               face_indices.push_back(v2);

               vertices.push_back(vt0);
               vertices.push_back(vt1);
               vertices.push_back(vt2);

               for (auto i = 3u; i < face.size(); i++)
               {
                  v1 = v2;
                  v2 = face[i];

                  vt1 = vt2;
                  vt2 = vertices[i];

                  face_indices.push_back(v0);
                  face_indices.push_back(v1);
                  face_indices.push_back(v2);

                  vertices.push_back(vt0);
                  vertices.push_back(vt1);
                  vertices.push_back(vt2);
               }
            }
            else
            {
               for (auto i = 0u; i < face.size(); i++)
               {
                  face_indices.push_back(face[i]);
                  vertices.push_back(vertices[i]);
               }
            }

            faces.push_back(face_indices);
         }
      }

      getline(obj_stream, line);
   }

   obj_stream.close();

   // std::cout << "Loaded mesh from: " << filename << std::endl;
   // std::cout << " " << points.size()     << " points"      << std::endl;
   // std::cout << " " << faceCount         << " faces"       << std::endl;
   // std::cout << " " << faces.size() / 3  << " triangles."  << std::endl;
   // std::cout << " " << normals.size()    << " normals"     << std::endl;
   // std::cout << " " << uvs.size()        << " uvs"         << std::endl;
}

void Mesh::writeVerticesToImage(
   const std::vector<b2Vec2>& points,
   const std::vector<std::vector<uint32_t>>& faces,
   const sf::Vector2i& textureSize,
   const std::filesystem::path& imagePath,
   float scale
)
{
   sf::RenderTexture render_texture;
   if (!render_texture.create(static_cast<uint32_t>(textureSize.x), static_cast<uint32_t>(textureSize.y)))
   {
      std::cout << "failed to create render texture" << std::endl;
      return;
   }

   render_texture.clear();

   for (const auto& face : faces)
   {
      sf::VertexArray poly(sf::LineStrip, face.size() + 1);

      auto i = 0u;
      for (const auto vertex_index : face)
      {
         const auto& pos = points[vertex_index];

         poly[i].color = sf::Color::Red;
         poly[i].position = sf::Vector2f{pos.x * scale, pos.y * scale};
         i++;
      }

      // close poly
      const auto& pos = points[face[0]];
      poly[face.size()].color = sf::Color::Red;
      poly[face.size()].position = sf::Vector2f{pos.x * scale, pos.y * scale};

      render_texture.draw(&poly[0], face.size() + 1, sf::LineStrip);
   }

   render_texture.display();

   // get the target texture (where the stuff has been drawn)
   const sf::Texture& texture = render_texture.getTexture();
   texture.copyToImage().saveToFile(imagePath.string());
}
