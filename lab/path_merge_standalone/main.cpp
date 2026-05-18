#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include <math.h>

#include "painterpath.h"
#include "pointf.h"

namespace
{
constexpr auto triangulate = false;
}

struct Vec3
{
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
};

struct Vec2
{
   float x = 0.0f;
   float y = 0.0f;
};

struct Vertex
{
   uint32_t _index_pos = 0;
   uint32_t _index_normal = 0;
   uint32_t _index_texcoord = 0;
};

void readObj(const std::string& filename, std::vector<Vec3>& points, std::vector<std::vector<uint32_t>>& faces)
{
   std::vector<Vertex> vertices;
   std::vector<Vec3> normals;
   std::vector<Vec2> uvs;

   auto trim_string = [](std::string& str)
   {
      const char* white_space = " \t\n\r";
      size_t location = str.find_first_not_of(white_space);
      str.erase(0, location);
      location = str.find_last_not_of(white_space);
      str.erase(location + 1);
   };

   auto face_count = 0u;

   std::ifstream obj_stream(filename, std::ios::in);

   if (!obj_stream)
   {
      std::cerr << "unable to open file: " << filename << std::endl;
      return;
   }

   std::string line, token;

   getline(obj_stream, line);

   while (!obj_stream.eof())
   {
      trim_string(line);

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
            points.push_back({x, y, z});
         }
         else if (token == "vt")
         {
            float s = 0.0f;
            float t = 0.0f;

            line_stream >> s >> t;
            uvs.push_back({s, t});
         }
         else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            line_stream >> x >> y >> z;
            normals.push_back({x, y, z});
         }
         else if (token == "f")
         {
            face_count++;

            std::vector<uint32_t> face;

            size_t slash_1 = 0;
            size_t slash_2 = 0;

            while (line_stream.good())
            {
               std::string vert_string;
               line_stream >> vert_string;

               auto index_pos = 0u;
               auto index_normal = 0u;
               auto index_texcoord = 0u;

               slash_1 = vert_string.find("/");

               if (slash_1 == std::string::npos)
               {
                  index_pos = static_cast<uint32_t>(atoi(vert_string.c_str()) - 1);
               }
               else
               {
                  slash_2 = vert_string.find("/", slash_1 + 1);
                  index_pos = static_cast<uint32_t>(atoi(vert_string.substr(0, slash_1).c_str()) - 1);

                  if (slash_2 > slash_1 + 1)
                  {
                     index_texcoord = static_cast<uint32_t>(atoi(vert_string.substr(slash_1 + 1, slash_2).c_str()) - 1);
                  }

                  index_normal = static_cast<uint32_t>(atoi(vert_string.substr(slash_2 + 1, vert_string.length()).c_str()) - 1);
               }

               Vertex vertex;
               vertex._index_pos = index_pos;
               vertex._index_normal = index_normal;
               vertex._index_texcoord = index_texcoord;

               face.push_back(index_pos);
               vertices.push_back(vertex);
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.

            std::vector<uint32_t> face_indices;
            if (face.size() > 3 && triangulate)
            {
               auto base_vertex_index = face[0];
               auto prev_vertex_index = face[1];
               auto curr_vertex_index = face[2];

               Vertex base_triangle_vertex = vertices[0];
               Vertex prev_triangle_vertex = vertices[1];
               Vertex curr_triangle_vertex = vertices[2];

               face_indices.push_back(base_vertex_index);
               face_indices.push_back(prev_vertex_index);
               face_indices.push_back(curr_vertex_index);

               vertices.push_back(base_triangle_vertex);
               vertices.push_back(prev_triangle_vertex);
               vertices.push_back(curr_triangle_vertex);

               for (auto fan_index = 3u; fan_index < face.size(); fan_index++)
               {
                  prev_vertex_index = curr_vertex_index;
                  curr_vertex_index = face[fan_index];

                  prev_triangle_vertex = curr_triangle_vertex;
                  curr_triangle_vertex = vertices[fan_index];

                  face_indices.push_back(base_vertex_index);
                  face_indices.push_back(prev_vertex_index);
                  face_indices.push_back(curr_vertex_index);

                  vertices.push_back(base_triangle_vertex);
                  vertices.push_back(prev_triangle_vertex);
                  vertices.push_back(curr_triangle_vertex);
               }
            }
            else
            {
               for (auto face_vertex_index = 0u; face_vertex_index < face.size(); face_vertex_index++)
               {
                  face_indices.push_back(face[face_vertex_index]);
                  vertices.push_back(vertices[face_vertex_index]);
               }
            }

            faces.push_back(face_indices);
         }
      }

      getline(obj_stream, line);
   }

   obj_stream.close();

   // std::cout << "Loaded mesh from: " << filename << std::endl;
   //
   // std::cout << " " << points.size()     << " points"      << std::endl;
   // std::cout << " " << faceCount         << " faces"       << std::endl;
   // std::cout << " " << faces.size() / 3  << " triangles."  << std::endl;
   // std::cout << " " << normals.size()    << " normals"     << std::endl;
   // std::cout << " " << uvs.size()        << " uvs"         << std::endl;
}

void writeObj(const std::string& filename, const std::vector<Vec3>& vertices, const std::vector<std::vector<uint32_t>>& faces)
{
   std::ofstream out(filename);

   out.setf(std::ios::fixed);
   for (const auto& mesh_vertex : vertices)
   {
      out << std::setprecision(3) << "v " << mesh_vertex.x << " " << mesh_vertex.y << " " << 0.0f << std::endl;
   }

   out << std::endl;

   for (const auto& face : faces)
   {
      out << "f ";
      for (const auto vertex_index : face)
      {
         out << vertex_index << " ";
      }
      out << std::endl;
   }

   out.close();
}

int main(int32_t argc, char** argv)
{
   if (argc != 3)
   {
      std::cout << "usage: " << argv[0] << " input_file.obj output_file.obj" << std::endl;
      exit(0);
   }

   // auto in = "C:\\git\\build\\layer_level.obj";
   // auto out = "C:\\git\\build\\layer_level_out.obj";
   auto input_file = argv[1];
   auto output_file = argv[2];

   std::vector<Vec3> points;
   std::vector<std::vector<uint32_t>> faces;

   readObj(input_file, points, faces);

   std::vector<std::vector<PointF>> polys;
   for (auto& face : faces)
   {
      std::vector<PointF> poly;

      for (auto face_vertex_index = 0u; face_vertex_index < face.size(); face_vertex_index++)
      {
         const auto face_index = face[face_vertex_index];
         const auto& point = points[face_index];
         poly.push_back({static_cast<double>(point.x), static_cast<double>(point.y)});
      }

      polys.push_back(poly);
   }

   PainterPath path;
   for (auto& poly : polys)
   {
      path.addPolygon(poly);
   }

   PainterPath simplified = path.simplified();

   auto simplified_polys = simplified.toSubpathPolygons();

   std::vector<std::vector<uint32_t>> simplified_faces;
   std::vector<Vec3> simplified_points;

   for (const auto& poly : simplified_polys)
   {
      std::vector<uint32_t> face;
      for (const auto& point : poly)
      {
         const auto& found_point = std::find_if(
            simplified_points.begin(),
            simplified_points.end(),
            [point](const Vec3& other)
            { return (fabs(point.x - static_cast<double>(other.x)) < 0.001 && fabs(point.y - static_cast<double>(other.y)) < 0.001); }
         );

         uint32_t vertex_index = 0;
         Vec3 new_point{static_cast<float>(point.x), static_cast<float>(point.y), 0.0f};
         if (found_point == simplified_points.end())
         {
            vertex_index = simplified_points.size();
            simplified_points.push_back(new_point);
         }
         else
         {
            vertex_index = found_point - simplified_points.begin();
         }

         face.push_back(vertex_index + 1);
      }

      simplified_faces.push_back(face);
   }

   writeObj(output_file, simplified_points, simplified_faces);

   std::cout << "optimised mesh written to '" << output_file << "', points: " << points.size() << " -> " << simplified_points.size()
             << ", faces: " << path.elementCount() << " -> " << simplified.elementCount()
             << ", factor: " << simplified_points.size() / static_cast<float>(points.size()) << std::endl;

   return 0;
}
