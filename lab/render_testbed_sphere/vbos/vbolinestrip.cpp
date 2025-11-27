#include "vbolinestrip.h"

#include "opengl/glutils.h"
#include "opengl/gl_current.h"


VboLineStrip::VboLineStrip(const std::vector<glm::vec3>& positions, Mode mode)
 : _mode(mode)
{
   std::vector<uint32_t> indices;
   uint32_t index = 0;
   for (auto& pos : positions)
   {
      (void)pos;
      indices.push_back(index++);
   }

   _element_count = indices.size();

   // make two buffers
   uint32_t handle[2];
   glGenBuffers(2, handle);

   // vertex buffer
   glGenVertexArrays(1, &_vao_handle);
   glBindVertexArray(_vao_handle);
   glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
   glBufferData(GL_ARRAY_BUFFER, 3 * positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)NULL + (0)));
   glEnableVertexAttribArray(0);

   // index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[1]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

   glBindVertexArray(0);
}


void VboLineStrip::render() const
{
   GLUtils::checkForOpenGLError(__FILE__,__LINE__);
   glBindVertexArray(_vao_handle);
   glDrawElements((_mode == Mode::Loop) ? GL_LINE_LOOP : GL_LINE_STRIP, _element_count, GL_UNSIGNED_INT, ((GLubyte*)NULL + (0)));
   GLUtils::checkForOpenGLError(__FILE__,__LINE__);
}
