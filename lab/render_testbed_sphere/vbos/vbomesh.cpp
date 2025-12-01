#include "vbomesh.h"

#include "opengl/glutils.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

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

   // Compute the tangent std::vector
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
   const char* white_space = " \t\n\r";
   size_t location;
   location = str.find_first_not_of(white_space);
   str.erase(0, location);
   location = str.find_last_not_of(white_space);
   str.erase(location + 1);
}

void center(std::vector<glm::vec3>& points)
{
   if (points.empty())
   {
      return;
   }

   auto max_point = points[0];
   auto min_point = points[0];

   // find the AABB
   std::ranges::for_each(
      points,
      [&max_point, &min_point](auto& point)
      {
         max_point.x = std::max(point.x, max_point.x);
         max_point.y = std::max(point.y, max_point.y);
         max_point.z = std::max(point.z, max_point.z);
         min_point.x = std::min(point.x, min_point.x);
         min_point.y = std::min(point.y, min_point.y);
         min_point.z = std::min(point.z, min_point.z);
      }
   );

   // center of the AABB
   const auto center =
      glm::vec3((max_point.x + min_point.x) / 2.0f, (max_point.y + min_point.y) / 2.0f, (max_point.z + min_point.z) / 2.0f);

   // translate center of the AABB to the origin
   for (GLuint i = 0; i < points.size(); ++i)
   {
      auto& point = points[i];
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
                  pIndex = atoi(vertex_string.c_str()) - 1;
               }
               else
               {
                  slash2 = vertex_string.find("/", slash1 + 1);
                  pIndex = atoi(vertex_string.substr(0, slash1).c_str()) - 1;
                  if (slash2 > slash1 + 1)
                  {
                     tcIndex = atoi(vertex_string.substr(slash1 + 1, slash2).c_str()) - 1;
                  }
                  nIndex = atoi(vertex_string.substr(slash2 + 1, vertex_string.length()).c_str()) - 1;
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

   if (_recenter_mesh)
   {
      center(points);
   }

   storeVbo(points, normals, texcoords, tangents, faces, vertices);

   std::cout << "Loaded mesh from: " << filename << "\n";
   std::cout << " " << points.size() << " points\n";
   std::cout << " " << face_count << " faces\n";
   std::cout << " " << faces.size() / 3 << " triangles\n";
   std::cout << " " << normals.size() << " normals\n";
   std::cout << " " << tangents.size() << " tangents\n";
   std::cout << " " << texcoords.size() << " texture coordinates\n";
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

   float* vertex_arr = new float[3 * vertex_count];
   float* normal_arr = new float[3 * vertex_count];
   float* texcoord_arr = nullptr;
   float* tangent_arr = nullptr;

   if (!texCoords.empty())
   {
      texcoord_arr = new float[2 * vertex_count];

      if (!tangents.empty())
      {
         tangent_arr = new float[4 * vertex_count];
      }
   }

   unsigned int* el = new unsigned int[vertex_count];

   int idx = 0;
   int tcIdx = 0;
   int tangIdx = 0;
   for (GLuint i = 0; i < vertices.size(); ++i)
   {
      vertex_arr[idx] = positions[vertices.at(i).pos_index].x;
      vertex_arr[idx + 1] = positions[vertices.at(i).pos_index].y;
      vertex_arr[idx + 2] = positions[vertices.at(i).pos_index].z;

      normal_arr[idx] = normals[vertices.at(i).normal_index].x;
      normal_arr[idx + 1] = normals[vertices.at(i).normal_index].y;
      normal_arr[idx + 2] = normals[vertices.at(i).normal_index].z;

      idx += 3;

      if (texcoord_arr != nullptr)
      {
         texcoord_arr[tcIdx] = texCoords[vertices.at(i).texcoord_index].x;
         texcoord_arr[tcIdx + 1] = texCoords[vertices.at(i).texcoord_index].y;
         tcIdx += 2;
      }

      if (tangent_arr != nullptr)
      {
         tangent_arr[tangIdx] = tangents[i].x;
         tangent_arr[tangIdx + 1] = tangents[i].y;
         tangent_arr[tangIdx + 2] = tangents[i].z;
         tangent_arr[tangIdx + 3] = tangents[i].w;
         tangIdx += 4;
      }
   }

   for (unsigned int i = 0; i < vertices.size(); ++i)
   {
      el[i] = i;
   }

   glGenVertexArrays(1, &_vao_handle);
   glBindVertexArray(_vao_handle);

   int buffer_count = 3;

   if (texcoord_arr != nullptr)
   {
      buffer_count++;
   }

   if (tangent_arr != nullptr)
   {
      buffer_count++;
   }

   GLuint elementBuffer = buffer_count - 1;

   GLuint handle[5];
   GLuint bufIdx = 0;
   glGenBuffers(buffer_count, handle);

   glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * vertex_count) * sizeof(float), vertex_arr, GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
   glEnableVertexAttribArray(0);  // Vertex position

   glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * vertex_count) * sizeof(float), normal_arr, GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
   glEnableVertexAttribArray(1);  // Vertex normal

   if (texcoord_arr != nullptr)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
      glBufferData(GL_ARRAY_BUFFER, (2 * vertex_count) * sizeof(float), texcoord_arr, GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
      glEnableVertexAttribArray(2);  // Texture coords
   }

   if (tangent_arr != nullptr)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
      glBufferData(GL_ARRAY_BUFFER, (4 * vertex_count) * sizeof(float), tangent_arr, GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)3, 4, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)nullptr + (0)));
      glEnableVertexAttribArray(3);  // Tangent std::vector
   }

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[elementBuffer]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * _faces * sizeof(unsigned int), el, GL_STATIC_DRAW);

   glBindVertexArray(0);

   // Clean up
   delete[] vertex_arr;
   delete[] normal_arr;
   delete[] texcoord_arr;
   delete[] tangent_arr;
   delete[] el;
}
