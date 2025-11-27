#include "vboquadstrip.h"

#include "opengl/glutils.h"
#include "opengl/gl_current.h"

#include <cstdio>
#include <cmath>

VboQuadStrip::VboQuadStrip(const std::vector<glm::vec3>& point_pairs/*, float width*/)
{
   /*
      2----3
      |  / |
      | /  |
      0----1
   */

   std::vector<float> positions;
   std::vector<uint32_t> indices;
   //   float* n = new float[3 * positions.size() * 2];
   //   float* tex = new float[2 * positions.size() * 2];

   // copy positions
   for (auto& vec : point_pairs)
   {
      positions.push_back(vec.x);
      positions.push_back(vec.y);
      positions.push_back(vec.z);

      // printf("%f|%f|%f\n",vec.x,vec.y,vec.z);
   }

   // 2 => 6
   // 3 => 12
   // 4 => 18

   // 0: 0,3,2 | 3,0,1
   // 1: 2,5,4 | 5,2,3
   auto vertex_index = 0;
   for (auto i = 0u; i < point_pairs.size() - 1; i++, vertex_index += 2)
   {
      indices.push_back(0 + vertex_index);
      indices.push_back(3 + vertex_index);
      indices.push_back(2 + vertex_index);

      indices.push_back(3 + vertex_index);
      indices.push_back(0 + vertex_index);
      indices.push_back(1 + vertex_index);
   }

   _element_count = indices.size();

   uint32_t handle[2];
   glGenBuffers(2, handle);

   glGenVertexArrays( 1, &_vao_handle);
   glBindVertexArray(_vao_handle);

   // vertex buffer
   glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
   glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
   glEnableVertexAttribArray(0);

   // glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
   // glBufferData(GL_ARRAY_BUFFER, 3 * (xdivs+1) * (zdivs+1) * sizeof(float), n, GL_STATIC_DRAW);
   // glVertexAttribPointer( (GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
   // glEnableVertexAttribArray(1);  // Vertex normal

   // glBindBuffer(GL_ARRAY_BUFFER, handle[2]);
   // glBufferData(GL_ARRAY_BUFFER, 2 * (xdivs+1) * (zdivs+1) * sizeof(float), tex, GL_STATIC_DRAW);
   // glVertexAttribPointer( (GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
   // glEnableVertexAttribArray(2);  // Texture coords

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[1]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

   glBindVertexArray(0);
}


void VboQuadStrip::render() const
{
   GLUtils::checkForOpenGLError(__FILE__,__LINE__);
   glBindVertexArray(_vao_handle);
   glDrawElements(GL_TRIANGLES,  _element_count, GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
   GLUtils::checkForOpenGLError(__FILE__,__LINE__);
}



