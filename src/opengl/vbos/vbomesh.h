#pragma once

#include "opengl/interfaces/drawable.h"

#include "opengl/gl_current.h"

#include "opengl/glm/glm.hpp"

#include <string>
#include <vector>

class VBOMesh : public Drawable
{
public:
   VBOMesh(
      const char* filename,
      float scale = 1.0f,
      bool recenter_mesh = false,
      bool load_texture_coordinates = false,
      bool generate_tangents = false
   );

   void render() const override;
   void loadObj(const char* fileName);

private:
   struct Vertex
   {
      int pos_index{};
      int normal_index{};
      int texcoord_index{};
   };

   void storeVbo(
      const std::vector<glm::vec3>& points,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& texcoords,
      const std::vector<glm::vec4>& tangents,
      const std::vector<GLuint>& elements,
      const std::vector<Vertex>& vertices
   );

   float _scale{1.0f};
   GLuint _faces{};
   GLuint _vao_handle{};

   bool _recenter_mesh{false};
   bool _load_texture{false};
   bool _generate_tangents{false};
};
