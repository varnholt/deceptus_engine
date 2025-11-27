#include "track.h"

#include "game/camera.h"
#include "game/shaderpool.h"

#include "opengl/glutils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


namespace
{
bool triangulate = false;
}


void Track::deserialize()
{
   deserialize("data/objects/track.obj", _points, _faces);
   scale();
   createVbo();
}


void Track::scale()
{
   constexpr auto scale = 5.0f;
   std::for_each(_points.begin(), _points.end(), [scale](auto& point){point *= scale;});
}


void Track::createVbo()
{
   /*
      2--*--3
      |   / |
      | /   |
      0--*--1
   */

   constexpr auto width = 12.0f;

   for (auto i = 0u; i < _points.size() - 1; i++)
   {
      const auto& pos_prev = (i == 0) ? _points.at(_points.size() - 1) : _points.at(i - 1);
      const auto& pos = _points[i];

      const auto dir = (pos - pos_prev);

      glm::vec3 n1{-dir.z, 0,  dir.x};
      glm::vec3 n2{ dir.z, 0, -dir.x};
      n1 = glm::normalize(n1);
      n2 = glm::normalize(n2);

      const auto p2 = pos + n1 * width;
      const auto p3 = pos + n2 * width;

      _positions.push_back(p2);
      _positions.push_back(p3);

      _ring_inner.push_back(p2);
      _ring_outer.push_back(p3);

      // std::cout << std::fixed << std::setw(11) << std::setprecision(3)  << p2.x << ", " << p2.z << " | " << p3.x << "," << p3.z << std::endl;
   }

   _positions.push_back(_positions[0]);
   _positions.push_back(_positions[1]);

   _vbo_track_quads = VboQuadStrip(_positions);
   _vbo_track_border_1 = VboLineStrip(_ring_inner, VboLineStrip::Mode::Loop);
   _vbo_track_border_2 = VboLineStrip(_ring_outer, VboLineStrip::Mode::Loop);
}


void Track::deserialize(
   const std::string& filename,
   std::vector<glm::vec3>& points,
   std::vector<std::vector<uint32_t>>& faces
)
{
   std::vector<Track::Vertex> vertices;
   std::vector<glm::vec2> normals;
   std::vector<glm::vec2> uvs;

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
             points.push_back(glm::vec3(x, y, z));
         }
         else if (token == "vt")
         {
             float s = 0.0f;
             float t = 0.0f;

             lineStream >> s >> t;
             uvs.push_back(glm::vec2(s, t));

         } else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            lineStream >> x >> y >> z;
            normals.push_back(glm::vec2(x, y));
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

               Track::Vertex vertex;
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

               Track::Vertex vt0 = vertices[0];
               Track::Vertex vt1 = vertices[1];
               Track::Vertex vt2 = vertices[2];

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


const std::vector<glm::vec3>& Track::getPoints() const
{
   return _points;
}


void Track::render()
{
   GLUtils::checkForOpenGLError(__FILE__,__LINE__);

   const auto& simple_shader = ShaderPool::getInstance().get("simple");
   simple_shader->use();

   mat4 mv = Camera::getInstance().getViewMatrix() * _model_matrix;
   simple_shader->setUniform("MVP", Camera::getInstance().getProjectionMatrix() * mv);

   GLUtils::checkForOpenGLError(__FILE__,__LINE__);

   simple_shader->setUniform("u_color", vec4(0.2f, 0.2f, 0.2f, 1.0f));
   _vbo_track_quads.render();

   simple_shader->setUniform("u_color", vec4(1.0f, 0.0f, 0.0f, 1.0f));
   _vbo_track_border_1.render();
   _vbo_track_border_2.render();

   GLUtils::checkForOpenGLError(__FILE__,__LINE__);
}


const std::vector<glm::vec3>& Track::getQuadPositions() const
{
   return _positions;
}


const std::vector<glm::vec3>& Track::getRingOuter() const
{
   return _ring_outer;
}


const std::vector<glm::vec3>& Track::getRingInner() const
{
   return _ring_inner;
}


const VboQuadStrip& Track::getVbo() const
{
   return _vbo_track_quads;
}
