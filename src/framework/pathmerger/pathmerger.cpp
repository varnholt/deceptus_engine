#include "pathmerger.h"

#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <sstream>

namespace PathMerge
{

namespace
{

constexpr auto triangulate = false;

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

}  // namespace

void PathMerger::loadObj(const std::string& filename)
{
   std::vector<Vec3> points;
   std::vector<std::vector<uint32_t>> faces;

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

      std::ifstream obj_stream(filename, std::ios::in);

      if (!obj_stream)
      {
         std::println(stderr, "unable to open file: {}", filename);
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
               std::vector<uint32_t> face;

               size_t slash_1 = 0;
               size_t slash_2 = 0;

               while (line_stream.good())
               {
                  std::string vert_string;
                  line_stream >> vert_string;

                  uint32_t index_pos = 0;
                  uint32_t index_normal = 0;
                  uint32_t index_texcoord = 0;

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

                  for (auto fan_index = size_t{3}; fan_index < face.size(); fan_index++)
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
                  for (auto face_vertex_index = size_t{0}; face_vertex_index < face.size(); face_vertex_index++)
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
   }

   _points_in = points.size();
   _faces_in = faces.size();

   for (auto& face : faces)
   {
      std::vector<PointF> polygon;
      for (const auto face_vertex_value : face)
      {
         const auto& point = points[face_vertex_value];
         polygon.push_back({static_cast<double>(point.x), static_cast<double>(point.y)});
      }
      addPolygon(polygon);
   }
}

void PathMerger::addPolygon(const std::vector<PointF>& polygon)
{
   _path.addPolygon(polygon);
}

std::vector<std::vector<PointF>> PathMerger::simplified() const
{
   return _path.simplified().toSubpathPolygons();
}

PathMerger::Stats PathMerger::saveObj(const std::string& filename) const
{
   const auto simplified_polys = simplified();

   std::vector<PointF> output_points;
   std::vector<std::vector<uint32_t>> output_faces;

   for (const auto& poly : simplified_polys)
   {
      std::vector<uint32_t> face;
      for (const auto& point : poly)
      {
         const auto found_point = std::ranges::find_if(
            output_points,
            [point](const PointF& other) { return std::fabs(point.x - other.x) < 0.001 && std::fabs(point.y - other.y) < 0.001; }
         );

         uint32_t vertex_index = 0;
         if (found_point == output_points.end())
         {
            vertex_index = static_cast<uint32_t>(output_points.size());
            output_points.push_back(point);
         }
         else
         {
            vertex_index = static_cast<uint32_t>(found_point - output_points.begin());
         }

         face.push_back(vertex_index + 1);
      }

      output_faces.push_back(face);
   }

   std::ofstream output_stream(filename);

   for (const auto& point : output_points)
   {
      output_stream << std::format("v {:.3f} {:.3f} 0.000\n", point.x, point.y);
   }

   output_stream << '\n';

   for (const auto& face : output_faces)
   {
      output_stream << "f";
      for (const auto vertex_index : face)
      {
         output_stream << ' ' << vertex_index;
      }
      output_stream << '\n';
   }

   return {_points_in, output_points.size(), _faces_in, output_faces.size()};
}

}  // namespace PathMerge
