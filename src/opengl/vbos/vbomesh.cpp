#include "vbomesh.h"

#include "opengl/glutils.h"

#include <algorithm>
#include <charconv>  // for std::from_chars
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>  // for std::string_view

namespace
{
void generateAveragedNormals(const std::vector<glm::vec3>& points, std::vector<glm::vec3>& normals, const std::vector<GLuint>& faces)
{
   for (auto i = 0; i < points.size(); i++)
   {
      normals.push_back(glm::vec3(0.0f));
   }

   for (auto i = 0; i < faces.size(); i += 3)
   {
      const glm::vec3& point_1 = points[faces[i]];
      const glm::vec3& point_2 = points[faces[i + 1]];
      const glm::vec3& point_3 = points[faces[i + 2]];

      glm::vec3 side_a = point_2 - point_1;
      glm::vec3 side_b = point_3 - point_1;
      glm::vec3 normalized_normal = glm::normalize(glm::cross(side_a, side_b));

      normals[faces[i]] += normalized_normal;
      normals[faces[i + 1]] += normalized_normal;
      normals[faces[i + 2]] += normalized_normal;
   }

   for (auto i = 0; i < normals.size(); i++)
   {
      normals[i] = glm::normalize(normals[i]);
   }
}

void generateTangents(
   const std::vector<glm::vec3>& points,
   const std::vector<glm::vec3>& normals,
   const std::vector<GLuint>& faces,
   const std::vector<glm::vec2>& texCoords,
   std::vector<glm::vec4>& tangents
)
{
   std::vector<glm::vec3> tan1Accum;
   std::vector<glm::vec3> tan2Accum;

   for (GLuint i = 0; i < points.size(); i++)
   {
      tan1Accum.push_back(glm::vec3(0.0f));
      tan2Accum.push_back(glm::vec3(0.0f));
      tangents.push_back(glm::vec4(0.0f));
   }

   // compute tangent
   for (GLuint i = 0; i < faces.size(); i += 3)
   {
      const glm::vec3& point_1 = points[faces[i]];
      const glm::vec3& point_2 = points[faces[i + 1]];
      const glm::vec3& point_3 = points[faces[i + 2]];

      const glm::vec2& tc1 = texCoords[faces[i]];
      const glm::vec2& tc2 = texCoords[faces[i + 1]];
      const glm::vec2& tc3 = texCoords[faces[i + 2]];

      glm::vec3 q1 = point_2 - point_1;
      glm::vec3 q2 = point_3 - point_1;
      float s1 = tc2.x - tc1.x, s2 = tc3.x - tc1.x;
      float t1 = tc2.y - tc1.y, t2 = tc3.y - tc1.y;
      float r = 1.0f / (s1 * t2 - s2 * t1);

      glm::vec3 tan1((t2 * q1.x - t1 * q2.x) * r, (t2 * q1.y - t1 * q2.y) * r, (t2 * q1.z - t1 * q2.z) * r);

      glm::vec3 tan2((s1 * q2.x - s2 * q1.x) * r, (s1 * q2.y - s2 * q1.y) * r, (s1 * q2.z - s2 * q1.z) * r);

      tan1Accum[faces[i]] += tan1;
      tan1Accum[faces[i + 1]] += tan1;
      tan1Accum[faces[i + 2]] += tan1;
      tan2Accum[faces[i]] += tan2;
      tan2Accum[faces[i + 1]] += tan2;
      tan2Accum[faces[i + 2]] += tan2;
   }

   for (GLuint i = 0; i < points.size(); ++i)
   {
      const glm::vec3& n = normals[i];
      glm::vec3& t1 = tan1Accum[i];
      glm::vec3& t2 = tan2Accum[i];

      // Gram-Schmidt orthogonalize
      tangents[i] = glm::vec4(glm::normalize(t1 - (glm::dot(n, t1) * n)), 0.0f);

      // Store handedness in w
      tangents[i].w = (glm::dot(glm::cross(n, t1), t2) < 0.0f) ? -1.0f : 1.0f;
   }

   tan1Accum.clear();
   tan2Accum.clear();
}

void trimString(std::string& str)
{
   if (str.empty())
      return;

   const auto whitespace = " \t\n\r";
   str.erase(0, str.find_first_not_of(whitespace));
   if (str.empty())
      return;
   str.erase(str.find_last_not_of(whitespace) + 1);
}

void center(std::vector<glm::vec3>& points)
{
   if (points.empty())
   {
      return;
   }

   auto max_point = points[0];
   auto min_point = points[0];

   // find the AABB - using ranges where available
   for (const auto& point : points)  // This is range-based for loop (C++11) but still C++23 compatible
   {
      max_point.x = std::max(point.x, max_point.x);
      max_point.y = std::max(point.y, max_point.y);
      max_point.z = std::max(point.z, max_point.z);
      min_point.x = std::min(point.x, min_point.x);
      min_point.y = std::min(point.y, min_point.y);
      min_point.z = std::min(point.z, min_point.z);
   }

   // center of the AABB
   const auto center =
      glm::vec3((max_point.x + min_point.x) / 2.0f, (max_point.y + min_point.y) / 2.0f, (max_point.z + min_point.z) / 2.0f);

   // translate center of the AABB to the origin
   for (auto& point : points)  // Range-based for loop - modern C++ style
   {
      point = point - center;
   }
}
}  // namespace

VBOMesh::VBOMesh(const char* filename, float scale, bool recenter_mesh, bool load_texture_coordinates, bool generate_tangents)
    : _scale(scale), _recenter_mesh(recenter_mesh), _load_texture(load_texture_coordinates), _generate_tangents(generate_tangents)
{
   loadObj(filename);
}

void VBOMesh::render() const
{
   glBindVertexArray(_vao_handle);
   glDrawElements(GL_TRIANGLES, 3 * _faces, GL_UNSIGNED_INT, ((GLubyte*)NULL + (0)));
}

void VBOMesh::loadObj(const char* filename)
{
   std::vector<glm::vec3> points;
   std::vector<glm::vec3> normals;
   std::vector<glm::vec2> texcoords;
   std::vector<GLuint> faces;
   std::vector<Vertex> vertices;

   int face_count = 0;

   std::ifstream input_filestream(filename, std::ios::in);

   if (!input_filestream)
   {
      std::cerr << "Unable to open OBJ file: " << filename << "\n";
      exit(1);
   }

   std::string line;
   std::string token;
   std::vector<int> face;
   std::vector<Vertex> face_vertices;

   getline(input_filestream, line);
   while (!input_filestream.eof())
   {
      trimString(line);
      if (!line.empty() && line.at(0) != '#')
      {
         std::istringstream lineStream(line);

         lineStream >> token;

         if (token == "v")
         {
            float x{};
            float y{};
            float z{};
            lineStream >> x >> y >> z;
            points.push_back(glm::vec3(x, y, z) * _scale);
         }
         else if (token == "vt" && _load_texture)
         {
            // Process texture coordinate
            float s{};
            float t{};
            lineStream >> s >> t;
            texcoords.push_back(glm::vec2(s, t));
         }
         else if (token == "vn")
         {
            float x{};
            float y{};
            float z{};
            lineStream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
         }
         else if (token == "f")
         {
            face_count++;

            // process face
            face.clear();
            face_vertices.clear();

            size_t slash1{};
            size_t slash2{};

            // int point, texCoord, normal;
            while (lineStream.good())
            {
               std::string vertex_string;
               lineStream >> vertex_string;
               int pIndex = -1;
               int nIndex = -1;
               int tcIndex = -1;

               slash1 = vertex_string.find("/");

               if (slash1 == std::string::npos)
               {
                  auto result = std::from_chars(vertex_string.data(), vertex_string.data() + vertex_string.size(), pIndex);
                  if (result.ec == std::errc{})
                  {
                     pIndex -= 1;  // adjust for 0-based indexing
                  }
                  else
                  {
                     // Handle error case - set pIndex to invalid value
                     pIndex = -1;
                  }
               }
               else
               {
                  slash2 = vertex_string.find("/", slash1 + 1);

                  // Process position index
                  std::string_view p_str = std::string_view(vertex_string).substr(0, slash1);
                  auto result = std::from_chars(p_str.data(), p_str.data() + p_str.size(), pIndex);
                  if (result.ec == std::errc{})
                  {
                     pIndex -= 1;  // adjust for 0-based indexing
                  }
                  else
                  {
                     // Handle error case
                     pIndex = -1;
                  }

                  if (slash2 > slash1 + 1)
                  {
                     std::string_view tc_str = std::string_view(vertex_string).substr(slash1 + 1, slash2 - slash1 - 1);
                     auto tc_result = std::from_chars(tc_str.data(), tc_str.data() + tc_str.size(), tcIndex);
                     if (tc_result.ec == std::errc{})
                     {
                        tcIndex -= 1;  // adjust for 0-based indexing
                     }
                     else
                     {
                        tcIndex = -1;
                     }
                  }

                  std::string_view n_str = std::string_view(vertex_string).substr(slash2 + 1);
                  auto n_result = std::from_chars(n_str.data(), n_str.data() + n_str.size(), nIndex);
                  if (n_result.ec == std::errc{})
                  {
                     nIndex -= 1;  // adjust for 0-based indexing
                  }
                  else
                  {
                     nIndex = -1;
                  }
               }

               if (pIndex == -1)
               {
                  std::cout << "Missing point index!!!\n";
               }
               else
               {
                  Vertex vertex;
                  vertex.pos_index = pIndex;
                  vertex.normal_index = nIndex;
                  vertex.texcoord_index = tcIndex;

                  face.push_back(pIndex);
                  face_vertices.push_back(vertex);
               }
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.
            if (face.size() > 3)
            {
               int v0 = face[0];
               int v1 = face[1];
               int v2 = face[2];

               Vertex vt0 = face_vertices[0];
               Vertex vt1 = face_vertices[1];
               Vertex vt2 = face_vertices[2];

               // first face
               faces.push_back(v0);
               faces.push_back(v1);
               faces.push_back(v2);

               vertices.push_back(vt0);
               vertices.push_back(vt1);
               vertices.push_back(vt2);

               for (GLuint i = 3; i < face.size(); i++)
               {
                  v1 = v2;
                  v2 = face[i];

                  vt1 = vt2;
                  vt2 = face_vertices[i];

                  faces.push_back(v0);
                  faces.push_back(v1);
                  faces.push_back(v2);

                  vertices.push_back(vt0);
                  vertices.push_back(vt1);
                  vertices.push_back(vt2);
               }
            }
            else
            {
               faces.push_back(face[0]);
               faces.push_back(face[1]);
               faces.push_back(face[2]);

               vertices.push_back(face_vertices[0]);
               vertices.push_back(face_vertices[1]);
               vertices.push_back(face_vertices[2]);
            }
         }
      }
      getline(input_filestream, line);
   }

   input_filestream.close();

   //    normals.clear();
   //    if (normals.size() == 0) {
   //        generateAveragedNormals(points,normals,faces);
   //    }

   std::vector<glm::vec4> tangents;
   if (_generate_tangents && !texcoords.empty())
   {
      generateTangents(points, normals, faces, texcoords, tangents);
   }

   // Calculate and output boundaries of the loaded mesh BEFORE centering
   // Use C++23 ranges to find min/max for each coordinate
   if (!points.empty())
   {
      // auto [min_x, max_x] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.x; });
      // auto [min_y, max_y] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.y; });
      // auto [min_z, max_z] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.z; });
      //
      // glm::vec3 min_point_before = glm::vec3(min_x->x, min_y->y, min_z->z);
      // glm::vec3 max_point_before = glm::vec3(max_x->x, max_y->y, max_z->z);

      if (_recenter_mesh)
      {
         center(points);
      }

      storeVbo(points, normals, texcoords, tangents, faces, vertices);

      // std::cout << "Loaded mesh from: " << filename << "\n";
      // std::cout << " " << points.size() << " points\n";
      // std::cout << " " << face_count << " faces\n";
      // std::cout << " " << faces.size() / 3 << " triangles\n";
      // std::cout << " " << normals.size() << " normals\n";
      // std::cout << " " << tangents.size() << " tangents\n";
      // std::cout << " " << texcoords.size() << " texture coordinates\n";
      //
      // std::cout << " Boundaries before centering:\n";
      // std::cout << "  Min: (" << min_point_before.x << ", " << min_point_before.y << ", " << min_point_before.z << ")\n";
      // std::cout << "  Max: (" << max_point_before.x << ", " << max_point_before.y << ", " << max_point_before.z << ")\n";
      // std::cout << "  Size: (" << (max_point_before.x - min_point_before.x) << ", " << (max_point_before.y - min_point_before.y) << ", "
      //           << (max_point_before.z - min_point_before.z) << ")\n";
   }
   else
   {
      // Handle empty points case
      if (_recenter_mesh)
      {
         center(points);
      }

      storeVbo(points, normals, texcoords, tangents, faces, vertices);

      // std::cout << "Loaded mesh from: " << filename << "\n";
      // std::cout << " " << points.size() << " points\n";
      // std::cout << " " << face_count << " faces\n";
      // std::cout << " " << faces.size() / 3 << " triangles\n";
      // std::cout << " " << normals.size() << " normals\n";
      // std::cout << " " << tangents.size() << " tangents\n";
      // std::cout << " " << texcoords.size() << " texture coordinates\n";
   }

   // Calculate and output boundaries of the loaded mesh AFTER centering
   if (!points.empty())
   {
      // auto [min_x, max_x] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.x; });
      // auto [min_y, max_y] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.y; });
      // auto [min_z, max_z] = std::ranges::minmax_element(points, {}, [](const glm::vec3& p) { return p.z; });
      //
      // glm::vec3 min_point_after = glm::vec3(min_x->x, min_y->y, min_z->z);
      // glm::vec3 max_point_after = glm::vec3(max_x->x, max_y->y, max_z->z);
      //
      // std::cout << " Boundaries after centering:\n";
      // std::cout << "  Min: (" << min_point_after.x << ", " << min_point_after.y << ", " << min_point_after.z << ")\n";
      // std::cout << "  Max: (" << max_point_after.x << ", " << max_point_after.y << ", " << max_point_after.z << ")\n";
      // std::cout << "  Size: (" << (max_point_after.x - min_point_after.x) << ", " << (max_point_after.y - min_point_after.y) << ", "
      //           << (max_point_after.z - min_point_after.z) << ")\n";
   }
}

void VBOMesh::storeVbo(
   const std::vector<glm::vec3>& positions,
   const std::vector<glm::vec3>& normals,
   const std::vector<glm::vec2>& texCoords,
   const std::vector<glm::vec4>& tangents,
   const std::vector<GLuint>& elements,
   const std::vector<Vertex>& vertices
)
{
   const auto vertex_count = GLuint(vertices.size());
   _faces = GLuint(elements.size() / 3);

   auto vertex_buffer = std::make_unique<float[]>(3 * vertex_count);
   auto normal_buffer = std::make_unique<float[]>(3 * vertex_count);
   std::unique_ptr<float[]> texcoord_buffer;
   std::unique_ptr<float[]> tangent_buffer;

   if (!texCoords.empty())
   {
      texcoord_buffer = std::make_unique<float[]>(2 * vertex_count);

      if (!tangents.empty())
      {
         tangent_buffer = std::make_unique<float[]>(4 * vertex_count);
      }
   }

   auto element_buffer = std::make_unique<unsigned int[]>(vertex_count);

   int idx = 0;
   int tcIdx = 0;
   int tangIdx = 0;
   for (GLuint i = 0; i < vertices.size(); ++i)
   {
      vertex_buffer[idx] = positions[vertices.at(i).pos_index].x;
      vertex_buffer[idx + 1] = positions[vertices.at(i).pos_index].y;
      vertex_buffer[idx + 2] = positions[vertices.at(i).pos_index].z;

      normal_buffer[idx] = normals[vertices.at(i).normal_index].x;
      normal_buffer[idx + 1] = normals[vertices.at(i).normal_index].y;
      normal_buffer[idx + 2] = normals[vertices.at(i).normal_index].z;

      idx += 3;

      if (texcoord_buffer)
      {
         texcoord_buffer[tcIdx] = texCoords[vertices.at(i).texcoord_index].x;
         texcoord_buffer[tcIdx + 1] = texCoords[vertices.at(i).texcoord_index].y;
         tcIdx += 2;
      }

      if (tangent_buffer)
      {
         tangent_buffer[tangIdx] = tangents[i].x;
         tangent_buffer[tangIdx + 1] = tangents[i].y;
         tangent_buffer[tangIdx + 2] = tangents[i].z;
         tangent_buffer[tangIdx + 3] = tangents[i].w;
         tangIdx += 4;
      }
   }

   for (unsigned int i = 0; i < vertices.size(); ++i)
   {
      element_buffer[i] = i;
   }

   glGenVertexArrays(1, &_vao_handle);
   glBindVertexArray(_vao_handle);

   int buffer_count = 3;

   if (texcoord_buffer)
   {
      buffer_count++;
   }

   if (tangent_buffer)
   {
      buffer_count++;
   }

   GLuint element_buffer_index = buffer_count - 1;

   GLuint handle[5];
   GLuint buf_idx = 0;
   glGenBuffers(buffer_count, handle);

   glBindBuffer(GL_ARRAY_BUFFER, handle[buf_idx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * vertex_count) * sizeof(float), vertex_buffer.get(), GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
   glEnableVertexAttribArray(0);  // Vertex position

   glBindBuffer(GL_ARRAY_BUFFER, handle[buf_idx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * vertex_count) * sizeof(float), normal_buffer.get(), GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
   glEnableVertexAttribArray(1);  // Vertex normal

   if (texcoord_buffer)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[buf_idx++]);
      glBufferData(GL_ARRAY_BUFFER, (2 * vertex_count) * sizeof(float), texcoord_buffer.get(), GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
      glEnableVertexAttribArray(2);  // Texture coords
   }

   if (tangent_buffer)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[buf_idx++]);
      glBufferData(GL_ARRAY_BUFFER, (4 * vertex_count) * sizeof(float), tangent_buffer.get(), GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)3, 4, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
      glEnableVertexAttribArray(3);  // Tangent std::vector
   }

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[element_buffer_index]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * _faces * sizeof(unsigned int), element_buffer.get(), GL_STATIC_DRAW);

   glBindVertexArray(0);

   // Smart pointers automatically handle memory cleanup
}
